
// This file is very dependent on globals in bw_fc_gn_r.js
//


// This is a kludge to get io thingy to another javaScript file.  We may
// even need to make this onConnect thing an array of functions.
//
if(typeof window['onConnect'] === undefined)
    var onConnect = false;




// Call with array of controlNames
//
// or
//
// Call with single string that is the controlName.
//
function session(scenario, controlNames, f0) {

    // TODO: add support for more than one controlName.

    if(!Array.isArray(controlNames)) {
        let controlName = controlNames;
        controlNames = [];
        controlNames[0] = controlName;
    }

    var gets = { };

    // The rx2 resampFactor is the reciprocal of this.
    //
    var resampFactor = 3.0; // transmitter resample rate factor.
    // This is the numberOfSamples into TX filter to the number of samples
    // feed to transmit.  Also a like thing for the receiver.

    var rate = 3.57; // This must match the values in ../webLauncher_programs/?_config

    controlNames.forEach(function(controlName) {

        gets[controlName] = {

            "resampFactor": function(value) {
                if(controlName !== 'rx2')
                    resampFactor = value;
                else
                    resampFactor = 1.0/value;

                console.log("Got: " + controlName + ':' + "resampFactor=" + resampFactor);

                if(serverBandwidthToSliderCB[controlName] !== undefined)
                    serverBandwidthToSliderCB[controlName](
                        rate/resampFactor);
            },
            /*
            "rate": function(value) {
                rate = value/1.0e6;
                console.log("------------------------got rate=" + rate);
                if(serverBandwidthToSliderCB[controlName] !== undefined)
                    serverBandwidthToSliderCB[controlName](rate/resampFactor);
            },
            */
            "freq": function(value) {
                if(serverFreqToSliderCB[controlName] !== undefined)
                    serverFreqToSliderCB[controlName](value/1.0e6);
            },
            "gain": function(value) {
                if(serverGainToSliderCB[controlName] !== undefined)
                    serverGainToSliderCB[controlName](value);
            }
        };
    });


    createSession(function(io) {


        if(onConnect)
            onConnect(io);


        io.Emit('getLauncherPrograms');
        io.On('receiveLauncherPrograms', function(programs) {


            // We know what the programs are so here we go
            if(scenario == 1) {
                // This is for the case when there is no interferer.
                io.Emit('launch', '/1_spectrumFeed',
                    '--bins ' + bins +
                    ' --device ' + spectrum1_usrp +
                    ' --freq ' + cfreq1, { runOne: true });
                io.Emit('launch', '/1_tx',
                    tx1_usrp + ' ' +
                    rate + ' ' +
                    cfreq1 + ' ' +
                    tx_resampFactor + ' ' +
                    gain,
                    { runOne: true });
            } else if(scenario == 2) {
                // This is for the case when there is an interferer.
                io.Emit('launch', '/2_spectrumFeed',
                    '--bins ' + bins +
                    ' --device ' + spectrum2_usrp +
                    ' --freq ' + cfreq2, { runOne: true });
                io.Emit('launch', '/2_tx',
                    tx2_usrp + ' ' +
                    rate + ' ' +
                    cfreq2 + ' ' +
                    tx_resampFactor + ' ' +
                    gain,
                    { runOne: true });
                io.Emit('launch', '/2_rx',
                    rx2_usrp + ' ' +
                    rate + ' ' +
                    cfreq2 + ' ' +
                    rx_resampFactor + ' ' +
                    gain,
                    { runOne: true });
                io.Emit('launch', '/2_tx_interferer',
                    tx2_interferer_usrp + ' ' +
                    rate + ' ' +
                    ifreq2 + ' ' +
                    tx_resampFactor + ' ' +
                    gain,
                    { runOne: true });
            }
        });


        if(scenario === 2) {
            /////////////////////////////////
            // This is for the Throughput Plot
            var t0 = 0;
            var totalBytesOut0 = 0;
            var y = [];
            for(let i=0; i<nPoints; ++i)
                y[i] = -1000.0;

            function checkThroughput(value) {

                // this code will not get executed unless this value for
                // 'liquidSync' "totalBytesOut" was sent.

                let t1 = Date.now();
                let dt = t1 - t0;

                if(dt < 0.7e+3 && dt > 0.3e+3 && value && totalBytesOut0) {

                    // value is in Bytes.  dt is in milliseconds.
                    // therefore value/dt is in K Bytes / second.
                    // 8*value/dt is in K Bit / second.
                    // and so:
                    // 8*(value - totalBytesOut0)/dt is in KBit/s
                    //
                    let bytesPerSecond = 8*(value - totalBytesOut0)/dt;
                    // This is about the same, because the rate of numbers
                    // coming in is once every 1/2 second.
                    //bytesPerSecond = (value - totalBytesOut0)/0.5;

                    //console.log(" ------------ rate=" + bytesPerSecond);

                    let y0 = [ bytesPerSecond/1000.0 ]; // Mbit/s
                    y = y0.concat(y);
                    y.pop();

                } else {
                    // We need to reset because we did not get good data.
                    // Not knowing we set the current bytesPerSecond to 0.
                    let y0 = [ 0 ];
                    y = y0.concat(y);
                    y.pop();
                }

                if(plotThroughputPlot)
                    plotThroughputPlot(y);
                t0 = t1;
                totalBytesOut0 = value;
            }


            /////////////////////////////////
        }


        io.On('getParameter', function(programName,
            controlName, parameter, value) {

            //console.log('getParameter Args=' + [].slice.call(arguments));

            if(serverModeToSliderCB &&
                    typeof serverModeToSliderCB[controlName] == 'function' &&
                    scenario === 2 && controlName === 'liquidFrame2'
                    && parameter === 'mode') {
                // We have a mod/err-corr scheme slider
                console.log('value=' + value);

                serverModeToSliderCB[controlName](value);
            }

            if(scenario === 2 && controlName === 'liquidSync' &&
                    parameter === "totalBytesOut" && plotThroughputPlot)
                checkThroughput(value);

            if(parameter !== 'freq' &&
                parameter !== 'gain' && parameter !== 'resampFactor')
                return;

            if(gets[controlName] !== undefined)
                gets[controlName][parameter](value);
        });


        io.On('addController', function(programName, set, get, image) {

           console.log('Got On("addController",) program="' +
                programName + '"' +
                '\n    set=' + JSON.stringify(set) +
                '\n    get=' + JSON.stringify(get) +
                '\n  image=' + image);



            controlNames.forEach(function(controlName) {


                if(set[controlName] !== undefined) {


                    if(sendModeCB && 
                        controlName === 'liquidFrame2' &&
                        set[controlName]['mode'] !== undefined) {

                        // We have a mod/err-corr scheme slider
                        sendModeCB[controlName] = function(md) {
                            io.Emit('setParameter', programName, controlName, 'mode', md);
                        };
                    }




                    if(set[controlName]['resampFactor'] !== undefined) {
                        sendBandwidthCB[controlName] = function(bw) {

                            resampFactor = rate/bw;
                            console.log("bw=" + bw +  " rate=" + rate +
                                    "  Setting  " + controlName +
                                    ":resampFactor=" + resampFactor);

                            if(controlName !== 'rx2')
                                io.Emit('setParameter', programName, controlName,
                                        'resampFactor', resampFactor);
                            else
                                // For rx2 we invert the resampFactor
                                io.Emit('setParameter', programName, controlName,
                                        'resampFactor', 1.0/resampFactor);
                        }
                    }

                    if(set[controlName]['freq'] !== undefined) {
                        sendFreqCB[controlName] = function(freq) {
                            console.log("Setting " + controlName + " freq=" + freq);
                            io.Emit('setParameter', programName, controlName, 'freq', 1.0e6 * freq);
                        }
                    }
                    if(set[controlName]['gain'] !== undefined) {
                        sendGainCB[controlName] = function(gain) {
                            console.log("Setting " + controlName + " gain=" + gain);
                            io.Emit('setParameter', programName, controlName, 'gain', gain);
                        }
                    }
                }
            });
        });


        // TODO: This argument parsing crap needs to be fixed, on the server.
        //
        function spec_updateCB(xxx) {

            var args = [].slice.call(arguments);
            // This is f*cked up.
            args = args[0];

            var id = args.shift(),
                cFreq = args.shift(),
                bandwidth = args.shift(),
                updatePeriod = args.shift();

            var y = args.shift();
            //console.log('plotSpectrum: ' + y );
            if(plotSpectrum)
                plotSpectrum(y);
        }

        var spc = spectrumFeeds(io, {
            // newSpectrum callback
            newSpectrum: function(id, address) {  
                console.log('newSpectrum:' + [...arguments]);
                console.log('ADDRESS=' + address);
                console.log("SUBSCRIBING TO: " + id);

                let addr = address.split(":")[1];

                if(scenario == 1 && addr === spectrum1_usrp)
                    spc.subscribe(id, spec_updateCB);
                else if(scenario == 2 && addr === spectrum2_usrp)
                    spc.subscribe(id, spec_updateCB);
            },

            spectrumEnd: function() {
                console.log('spectrumEnd:' + [...arguments]);
            }
        });


    }, { addAdminPanel: false, showHeader: false, loadCRTScss: false});
}


// This file is very dependent on globals in bw_fc_gn_r.js
//


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

    controlNames.forEach(function(controlName) {

        gets[controlName] = {

            "rate": function(value) {
                if(serverBandwidthToSliderCB[controlName] !== undefined)
                    serverBandwidthToSliderCB[controlName](value * 1.0e-6);
            },
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

        io.Emit('getLauncherPrograms');
        io.On('receiveLauncherPrograms', function(programs) {


            // We know what the programs are so here we go
            if(scenario == 1) {
                // This is for the case when there is no interferer.
                io.Emit('launch', '/1_spectrumFeed',
                    '--bins ' + bins +
                    ' --freq ' + f0/1.0e6, { runOne: true });
                io.Emit('launch', '/1_tx', '', { runOne: true });
            } else if(scenario == 2) {
                // This is for the case when there is an interferer.
                io.Emit('launch', '/2_spectrumFeed',
                    '--bins ' + bins +
                    ' --freq ' + f0/1.0e6, { runOne: true });
                io.Emit('launch', '/2_tx', '', { runOne: true });
                io.Emit('launch', '/2_rx', '', { runOne: true });
                io.Emit('launch', '/2_tx_interferer', '', { runOne: true });
            }
        });


        if(scenario === 2) {
            /////////////////////////////////
            // This is for the ThroughputPlot
            var t0 = 0;
            var totalBytesOut0 = 0;
            var y = [];
            for(let i=0; i<nPoints; ++i)
                y[i] = 0;

            function checkThroughput(value) {

                // this code will not get executed unless this value for
                // 'liquidSync' "totalBytesOut" was sent.

                let t1 = Date.now();
                let dt = t1 - t0;

                if(dt < 0.7e+3 && dt > 0.3e+3 && value && totalBytesOut0) {

                    let bytesPerSecond = (value - totalBytesOut0)/dt;
                    // This is about the same, because the rate of numbers
                    // coming in is once every 1/2 second.
                    //bytesPerSecond = (value - totalBytesOut0)/0.5;

                    //console.log(" ------------ rate=" + bytesPerSecond);

                    let y0 = [ bytesPerSecond ];
                    y = y0.concat(y);
                    y.pop();

                } else {
                    // We need to reset because we did not get good data.
                    // Not knowing we set the current bytesPerSecond to 0.
                    let y0 = [ 0 ];
                    y = y0.concat(y);
                    y.pop();
                }

                plotThroughputPlot(y);
                t0 = t1;
                totalBytesOut0 = value;
            }
        /////////////////////////////////
        }


        io.On('getParameter', function(programName,
            controlName, parameter, value) {

            //console.log('getParameter Args=' + [].slice.call(arguments));

            if(scenario === 2 && controlName === 'liquidSync' && parameter === "totalBytesOut")
                checkThroughput(value);

            if(parameter !== 'rate' && parameter !== 'freq' && parameter !== 'gain')
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

                    if(set[controlName]['rate'] !== undefined) {
                        sendBandwidthCB[controlName] = function(bw) {
                            let rate = 1.0e6 * bw;
                            console.log("Setting  " + controlName + " bw=" + bw + " => rate=" + rate);
                            io.Emit('setParameter', programName, controlName, 'rate',rate);
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

            plotSpectrum(y);
        }

        var spc = spectrumFeeds(io, {
            // newSpectrum callback
            newSpectrum: function(id, address) {  
                console.log('newSpectrum:' + [...arguments]);
                console.log('ADDRESS=' + address);
                console.log("SUBSCRIBING TO: " + id);

                if(scenario == 1 && address === "peach:addr=192.168.40.108")
                    spc.subscribe(id, spec_updateCB);
                else if(scenario == 2 && address === "peach:addr=192.168.40.111")
                    spc.subscribe(id, spec_updateCB);
            },

            spectrumEnd: function() {
                console.log('spectrumEnd:' + [...arguments]);
            }
        });


    }, { addAdminPanel: false, showHeader: false, loadCRTScss: false});
}

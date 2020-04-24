
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

    if(Array.isArray(controlNames)) {
        var controlName = controlNames[0];
    } else {
        var controlName = controlNames;
    }


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

        io.On('getParameter', function(programName,
            _controlName, parameter, value) {
            if(_controlName === controlName && parameter === 'rate') {
                console.log("got rate=" + value * 1.0e-6);
                if(serverBandwidthToSliderCB[controlName] !== undefined)
                    serverBandwidthToSliderCB[controlName](value * 1.0e-6);
            } else if(_controlName === controlName && parameter === 'freq') {
                console.log("got freq=" + value/1.0e6);
                if(serverFreqToSliderCB[controlName] !== undefined)
                    serverFreqToSliderCB[controlName](value/1.0e6);
            } else if(_controlName === controlName && parameter === 'gain') {
                console.log("got gain=" + value);
                if(serverGainToSliderCB[controlName] !== undefined)
                    serverGainToSliderCB[controlName](value);
            }
        });


        io.On('addController', function(programName, set, get, image) {

           console.log('Got On("addController",) program="' +
                programName + '"' +
                '\n    set=' + JSON.stringify(set) +
                '\n    get=' + JSON.stringify(get) +
                '\n  image=' + image);


            if(set[controlName] !== undefined &&
                    set[controlName]['rate'] !== undefined) {
                sendBandwidthCB[controlName] = function(bw) {
                    let rate = 1.0e6 * bw;
                    console.log("Setting bw=" + bw + " => rate=" + rate);
                    io.Emit('setParameter', programName, controlName, 'rate',rate);
                }
            }
            if(set[controlName] !== undefined &&
                    set[controlName]['freq'] !== undefined) {
                sendFreqCB[controlName] = function(freq) {
                    console.log("Setting freq=" + freq);
                    io.Emit('setParameter', programName, controlName, 'freq', 1.0e6 * freq);
                }
            }
            if(set[controlName] !== undefined &&
                    set[controlName]['gain'] !== undefined) {
                sendGainCB[controlName] = function(gain) {
                    console.log("Setting gain=" + gain);
                    io.Emit('setParameter', programName, controlName, 'gain', gain);
                }
            }
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

// Note: In order to keep webDesktop and scope usable by other software
// projects we cannot use require() in them so we must put a longer list
// of dependences in this CRTS specific code.

require('/scope/scope.css');
require('/js/session.js');
require('/scope/scope.js');



function createSpectrumFeeds(io, createDisplay = null) {


    if(typeof(createDisplay) !== 'function') {
        createDisplay = function() {
            this.destroy = function() {};
            this.draw = function(x,y) {
                console.log("DRAW: " + y);
            };
        };
    }

    var spectrums = {};

    io.On("spectrumUpdate", function(id, cFreq, bandwidth,
            updatePeriod, data) {

        if(spectrums[id] === undefined) {
            io.Emit('spectrumUnsubscribe', id);
            return;
        }

        let spectrum = spectrums[id];
        let bins = data.length;

        /*
        console.log('got spectrumFeed(' + id + '): ' +
                    ' cFreq=' + cFreq + ' bandwidth=' + bandwidth +
                    ' bins=' + bins + ' period=' +  updatePeriod +
                    ' amplitudes=' + data + "\n\n");
                    */

        // We do not need to generate a new x array unless parameters have
        // changed.
        if(spectrum.cFreq !== cFreq ||
                spectrum.bandwidth !== bandwidth ||
                spectrum.bins !== bins) {
            let x = [];
            let deltaFreq = bandwidth/(bins-1);
            let startX = cFreq - bandwidth/(bins-1);

            if(bins === 1) {
                startX = cFreq;
                deltaFreq = 0.0;
            }

            for(let i=0 ;i<bins; ++i) {
                x[i] = startX + i * deltaFreq;
            }
            spectrum.x = x;
            spectrum.cFreq = cFreq;
            spectrum.bandwidth = bandwidth;
            spectrum.bins = bins;
        }

        spectrums[id].draw(spectrum.x, data);
    });


    io.On('spectrumEnd', function(id) {
        if(spectrums[id] !== undefined) {
            spectrums[id].destroy();
            delete spectrums[id];
        }
    });

    io.On('newSpectrum', function(id, description)  {

        console.log('Got "newSpectrum" [' + id + ']: "' +
                description + '"');

        assert(spectrums[id] === undefined, "We have spectrum id="
                + id + " already");

        spectrums[id] = new createDisplay(description, function() {
            io.Emit('spectrumUnsubscribe', id);
        });

        io.Emit('spectrumSubscribe', id);
    });
}

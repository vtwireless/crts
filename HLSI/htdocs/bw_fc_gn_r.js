
// 2. Use the margin convention practice
var margin = {top: 50, right: 50, bottom: 50, left: 50}
  , width  = 920 - margin.left - margin.right  // Use the window's width
  , height = 320 - margin.top - margin.bottom; // Use the window's height


var fs = 4e6, f0 = 915.5e6; // sample rate, center frequency
var fc = 0;    // some kind of relative frequency
var bw = 0.25; // relative to plot width filter bandwidth
var bins = 200; // number of fft points per plot or number of datapoints
var gn = 31.5; // gain in db

// determine time and frequency scale/units
var [scale_freq,units_freq] = scale_units(f0+fs/2,0.1); // freq scale

// 5. X scale will use the index of our data
var fScale = d3.scaleLinear().domain([(f0-0.5*fs)*scale_freq, (f0+0.5*fs)*scale_freq]).range([0, width]);

// 6. Y scale will use the randomly generate number
var vScale = d3.scaleLinear().domain([ -1.1,  1.1]).range([height, 0]);
var pScale = d3.scaleLinear().domain([-90, -20]).range([height, 0]);

// 7. d3's line generator
    //.curve(d3.curveMonotoneX) // apply smoothing to the line (comment out to remove smoothing)
var linef = d3.line()
    .x(function(d, i) { return fScale((f0+(i/bins-0.5)*fs)*scale_freq); })  // map frequency
    .y(function(d)    { return pScale(d.y);        }); // map PSD

// 8. An array of objects of length N. Each object has key -> value pair, the key being "y" and the value is a random number
var datai = {};
var dataq = {};
var dataf = d3.range(0,bins-1).map(function(f) { return {"y": 0 } })

// create SVG objects
var svgf = svg_create(margin, width, height, fScale, pScale);

// add labels
svg_add_labels(svgf, margin, width, height, "Frequency ("+units_freq+"Hz)", "Power Spectra Density (dB)");

// clip paths
svgf.append("clipPath").attr("id","clipf").append("rect").attr("width",width).attr("height",height);

// 9. Append the path, bind the data, and call the line generator
var pathf = svgf.append("path")
    .attr("clip-path","url(#clipf)")
    .datum(dataf)
    .attr("class", "stroke-med no-fill stroke-red")
    .attr("d", linef);




var sendBandwidthCB = false;
var sendFreqCB = false;
var sendGainCB = false;

/////////////////////////// BANDWIDTH ////////////////////////////////////

if(document.querySelector('#bw')) {

    function serverBandwidthToSliderCB(bandwidth) {

        // This should update the bandwidth slider from the web.

        // bw is a slider relative scale of bandwidth
        bw = bandwidth/(fs*scale_freq);

        document.querySelector('#bw').value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";
        document.querySelector('#bandwidth').value = bw;

        console.log("bw=" + bw);
    }


    sliderBandwidthCB();

    function sliderBandwidthCB(band) {

        // bw is a slider relative scale of bandwidth
        if (band!=null) { bw = band; }

        document.querySelector('#bw').value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";

        if(sendBandwidthCB) 
            // Send to the web server.
            sendBandwidthCB(bw*fs*scale_freq);

        console.log("--- bw=" + bw);
    }
}
///////////////////////////////////////////////////////////////////////////


//////////////////////////// FREQUENCY ////////////////////////////////////

if(document.querySelector('#fc')) {
    
    function serverFreqToSliderCB(freq) {

        // This should update the frequency slider from the web.
        //
        // fc is a slider relative scale of frequency
        fc = freq/(fs*scale_freq) - f0/fs;
        //
        document.querySelector('#fc').value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";
        document.querySelector('#frequency').value = fc;
        console.log("fc=" + fc);
    }


    sliderFreqCB();

    function sliderFreqCB(fc_in) {

        // fc is a slider relative scale of frequency
        if(fc_in!=null) { fc = fc_in; }

        document.querySelector('#fc').value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";

        if(sendFreqCB) 
            // Send freq to the web server.
            sendFreqCB((f0+fc*fs)*scale_freq);

        console.log("--- fc=" + fc);
    }
}
///////////////////////////////////////////////////////////////////////////


/////////////////////////////// GAIN //////////////////////////////////////

if(document.querySelector('#gn')) {

    function serverGainToSliderCB(gain) {

        // This should update the gain slider from the web.
        //
        // gn is a slider relative scale of gain.  Looks like it's also gain
        // in db.
        //
        gn = gain;
        //
        document.querySelector('#gn').value = d3.format(".1f")(gn) + " dB";
        document.querySelector('#gain').value = gn;
        console.log("gn=" + gn);
    }


    sliderGainCB();

    function sliderGainCB(gn_in) {

        // gn is a slider relative scale of gain.  Looks like it's also gain
        // in db.
        //
        if(gn_in!=null) { gn = gn_in; }

        document.querySelector('#gn').value = d3.format(".1f")(gn) + " dB";

        if(sendGainCB) 
            // Send gain to the web server.
            sendGainCB(gn);

        console.log("--- gn=" + gn);
    }
}
///////////////////////////////////////////////////////////////////////////



onload = function() {

    createSession(function(io) {

        io.Emit('getLauncherPrograms');
        io.On('receiveLauncherPrograms', function(programs) {
            // We know what the programs are so here we go:
            io.Emit('launch', '/spectrumFeed',
                '--bins ' + bins +
                ' --freq ' + f0/1.0e6, { runOne: true });
            io.Emit('launch', '/tx', '', { runOne: true });
        });

        io.On('getParameter', function(programName,
            controlName, parameter, value) {
            if(controlName === 'tx' && parameter === 'rate') {
                console.log("got rate=" + value * 1.0e-6);
                if(serverBandwidthToSliderCB)
                    serverBandwidthToSliderCB(value * 1.0e-6);
            }
            if(controlName === 'tx' && parameter === 'freq') {
                console.log("got freq=" + value/1.0e6);
                if(serverFreqToSliderCB)
                    serverFreqToSliderCB(value/1.0e6);
            }
            if(controlName === 'tx' && parameter === 'gain') {
                console.log("got gain=" + value);
                if(serverGainToSliderCB)
                    serverGainToSliderCB(value);
            }
        });


        io.On('addController', function(programName, set, get, image) {

           console.log('Got On("addController",) program="' +
                programName + '"' +
                '\n    set=' + JSON.stringify(set) +
                '\n    get=' + JSON.stringify(get) +
                '\n  image=' + image);

            if(set['tx'] !== undefined &&
                    set['tx']['rate'] !== undefined) {
                sendBandwidthCB = function(bw) {
                    let rate = 1.0e6 * bw;
                    console.log("Setting bw=" + bw + " => rate=" + rate);
                    io.Emit('setParameter', programName,'tx','rate',rate);
                }
            }
            if(set['tx'] !== undefined &&
                    set['tx']['freq'] !== undefined) {
                sendFreqCB = function(freq) {
                    console.log("Setting freq=" + freq);
                    io.Emit('setParameter', programName, 'tx', 'freq', 1.0e6 * freq);
                }
            }
            if(set['tx'] !== undefined &&
                    set['tx']['gain'] !== undefined) {
                sendGainCB = function(gain) {
                    console.log("Setting gain=" + gain);
                    io.Emit('setParameter', programName, 'tx', 'gain', gain);
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
    
            dataf = d3.range(0,bins-1).map(function(i) { 
                return {"y": 20*Math.log10(y[i]) }
            });
            pathf.datum(dataf).attr("d", linef);
        }

        var spc = spectrumFeeds(io, {
            // newSpectrum callback
            newSpectrum: function(id) {  
                console.log('newSpectrum:' + [...arguments]);
                console.log("SUBSCRIBING TO: " + id);
                spc.subscribe(id, spec_updateCB);
            },

            spectrumEnd: function() {
                console.log('spectrumEnd:' + [...arguments]);
            }
        });


    }, { addAdminPanel: false, showHeader: false, loadCRTScss: false});
};

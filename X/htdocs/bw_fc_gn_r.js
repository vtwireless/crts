
require("/X/session.js");

//////////////////////////////////////////////////////////////////
//
//                       CONFIGURATION
//
//  We made it so that these numbers are only set in this one
//  file.   They are passed as arguments to the launcher via
//  webSockets.
//
//  Changing these numbers in more than one file was causing us
//  pain.
//
//////////////////////////////////////////////////////////////////

///// Both scenarios /////
//
var rate = 3.57; // MHz
var gain = 31.5; // relative dB

// These next 2 must be reciprocals of each other:
var tx_resampFactor = 2.5;
var rx_resampFactor = 1.0/tx_resampFactor;


/////// Scenario 1 ///////

// starting center frequency
var cfreq1 = 915.5; // MHz

var tx1_usrp = "addr=192.168.40.107";

var spectrum1_usrp = "addr=192.168.40.108";


/////// Scenario 2 ///////

var cfreq2 = 919.5; // MHz

var tx2_usrp = "addr=192.168.40.112";

var rx2_usrp = "addr=192.168.40.211";

var spectrum2_usrp = "addr=192.168.40.111";

var tx2_interferer_usrp = "addr=192.168.40.212";

var ifreq2 = 918; // MHz


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////    
// Pick a scenario: 1 or 2
var scenario = (document.querySelector('#ign'))?2:1;

console.log(" ++++++++++++++++++ running scenario = " + scenario);


// Globals that are used in session.js and this file.

var serverBandwidthToSliderCB = {};
var sendBandwidthCB = {};
var serverFreqToSliderCB = {};
var serverModeToSliderCB = false;
var sendFreqCB = {};
var serverGainToSliderCB = {};
var sendGainCB = {};
var sendModeCB = false;
var bins = 512; // number of fft points per plot or number of datapoints


function plotSpectrum(y) {

    dataf = d3.range(0,bins-1).map(function(i) { 
            return {"y": 20*Math.log10(y[i]) }
    });
    pathf.datum(dataf).attr("d", linef);
}

var f0;

switch(scenario) {

    case 1:
        f0 = cfreq1 * 1.0e6; /*center Frequency of spectrum plot in Hz*/
        break;
    case 2:
        f0 = cfreq2 * 1.0e6; /*center Frequency of spectrum plot in Hz*/
        break;
}


onload = function() {

    switch(scenario) {

        case 1:

            session(scenario, 'tx1', f0);
            makeSlidersDisplay('tx1'/*controlName*/, 
                /*input/output element IDs: */
                'bandwidth', 'bw', 'frequency', 'fc', 'gain', 'gn');
            break;

        case 2:

            let throughPutMax = 3; // Mbit/s

            if(document.querySelector('#mode'))
                throughPutMax = 7; // Mbit/s

            makeThroughputPlot(throughPutMax);
            session(scenario, ['tx2', 'tx2_interferer', 'rx2', 'liquidFrame2'], f0);
            makeSlidersDisplay(['tx2', 'rx2']/*controlNames*/,
                /*input/output element IDs: */
                'bandwidth', 'bw', 'frequency', 'fc', 'gain', 'gn');
            makeSlidersDisplay('tx2_interferer'/*controlName*/,
                /*input/output element IDs: */
                'ibandwidth', 'ibw', 'ifrequency', 'ifc', 'igain', 'ign');
            if(document.querySelector('#mode'))
                // modulation/error-correction scheme slider
                addModSlider();
            break;
    }

};


var plotThroughputPlot;
var nPoints = 240; // samples kept
var dt = 0.5; // period between samples

function makeThroughputPlot(max=3) {

    var y = [];

    var xScale = d3.scaleLinear().domain([nPoints*dt, 0]).range([0, width]);

    var yScale = d3.scaleLinear().domain([0, max]).range([height, 0]);

    var linef = d3.line()
        .x(function(d, i) { return xScale(i*dt); })
        .y(function(d)    { return yScale(d.y); });


    var dataf = d3.range(0,bins-1).map(function(f) { return {"y": 0 } })

    var svgf = svg_create(margin, width, height, xScale, yScale);

    svg_add_labels(svgf, margin, width, height, "Past Time (s)", "Throughput (Mbit / s)");

    svgf.append("clipPath").attr("id","clipf").append("rect").attr("width",width).attr("height",height);

    var pathf = svgf.append("path")
        .attr("clip-path","url(#clipf)")
        .datum(dataf)
        .attr("class", "stroke-med no-fill stroke-red")
        .attr("d", linef);

    var i;
    for(i=0;i<nPoints;++i) {
        y[i] = 0;
    }

    function plot(y) {

        dataf = d3.range(0,nPoints).map(function(i) {
            //console.log(y[i]);
            return {"y": y[i] }
        });

        pathf.datum(dataf).attr("d", linef);
    }

    plot(y);

    plotThroughputPlot = plot
}


// Other Globals that I have not figured out yet:
//
// 2. Use the margin convention practice
var margin = {top: 50, right: 50, bottom: 50, left: 50}
  , width  = 920 - margin.left - margin.right  // Use the window's width
  , height = 320 - margin.top - margin.bottom; // Use the window's height


var fs = 4e6;  // sample rate scale


// determine time and frequency scale/units
var [scale_freq,units_freq] = scale_units(f0+fs/2,0.1); // freq scale

// 5. X scale will use the index of our data
var fScale = d3.scaleLinear().domain([(f0-0.5*fs)*scale_freq, (f0+0.5*fs)*scale_freq]).range([0, width]);

var pScale = d3.scaleLinear().domain([-90, -20]).range([height, 0]);

// 7. d3's line generator
var linef = d3.line()
    .x(function(d, i) { return fScale((f0+(i/bins-0.5)*fs)*scale_freq); })  // map frequency
    .y(function(d)    { return pScale(d.y);        }); // map PSD


// 8. An array of objects of length N. Each object has key -> value pair, the key being "y" and the value is a random number
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




function makeSlidersDisplay(controlName, bandwidthId, bwId, frequencyId, fcId, gainId, gnId) {


    if(!Array.isArray(controlName))
        var controlNames = [controlName];
    else
        var controlNames = controlName;


    var fc = 0;    // some kind of slider relative frequency
    var bw = 0.25; // relative to plot width filter bandwidth
    var gn = 31.5; // gain in db

/////////////////////////// BANDWIDTH ////////////////////////////////////


function addBandwidth(sliderId, outputId, controlNames) {


    controlNames.forEach(function(controlName) {
        serverBandwidthToSliderCB[controlName] = function(bandwidth) {

            // This should update the bandwidth slider from the web.

            // bw is a slider relative scale of bandwidth
            bw = bandwidth/(fs*scale_freq);

            document.querySelector('#'+outputId).value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";
            document.querySelector('#'+sliderId).value = bw;

            console.log("bw=" + bw);
        }
    });

    sliderBandwidthCB();

    function sliderBandwidthCB(band) {

        // bw is a slider relative scale of bandwidth
        if (band!=null) { bw = band; }

        document.querySelector('#'+outputId).value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";

        controlNames.forEach(function(name) {
            if(sendBandwidthCB[name] !== undefined)
                // Send to the web server.
                sendBandwidthCB[name](bw*fs*scale_freq);
        });

        console.log("--- bw=" + bw);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderBandwidthCB(parseFloat(document.querySelector('#'+sliderId).value));
    };
}

if(document.querySelector('#'+bwId) && document.querySelector('#'+bandwidthId))
    addBandwidth(bandwidthId, bwId, controlNames);


///////////////////////////////////////////////////////////////////////////


//////////////////////////// FREQUENCY ////////////////////////////////////


function addFreq(sliderId, outputId, controlNames) {
   

    controlNames.forEach(function(controlName) {
        serverFreqToSliderCB[controlName]= function(freq) {

            // This should update the frequency slider from the web.
            //
            // fc is a slider relative scale of frequency
            fc = freq/(fs*scale_freq) - f0/fs;
            //
            document.querySelector('#'+outputId).value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";
            document.querySelector('#'+sliderId).value = fc;
            console.log("fc=" + fc);
        }
    });

    sliderFreqCB();

    function sliderFreqCB(fc_in) {

        // fc is a slider relative scale of frequency
        if(fc_in!=null) { fc = fc_in; }

        document.querySelector('#'+outputId).value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";

        controlNames.forEach(function(name) {
            if(sendFreqCB[name] !== undefined) 
                // Send freq to the web server.
                sendFreqCB[name]((f0+fc*fs)*scale_freq);
        });

        console.log("--- fc=" + fc);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderFreqCB(parseFloat(document.querySelector('#'+sliderId).value));
    };

}

if(document.querySelector('#'+fcId) && document.querySelector('#'+frequencyId))
    addFreq(frequencyId, fcId, controlNames);


///////////////////////////////////////////////////////////////////////////


/////////////////////////////// GAIN //////////////////////////////////////


function addGain(sliderId, outputId, controlNames) {


    controlNames.forEach(function(controlName) {
        serverGainToSliderCB[controlName] = function(gain) {

            // This should update the gain slider from the web.
            //
            // gn is a slider relative scale of gain.  Looks like it's also gain
            // in db.
            //
            gn = gain;
            //
            document.querySelector('#'+outputId).value = d3.format(".1f")(gn) + " dB";
            document.querySelector('#'+sliderId).value = gn;
            console.log("gn=" + gn);
        }
    });


    sliderGainCB();

    function sliderGainCB(gn_in) {

        // gn is a slider relative scale of gain.  Looks like it's also gain
        // in db.
        //
        if(gn_in!=null) { gn = gn_in; }

        document.querySelector('#'+outputId).value = d3.format(".1f")(gn) + " dB";

        controlNames.forEach(function(name) {
            if(sendGainCB[name] !== undefined) 
                // Send gain to the web server.
                sendGainCB[name](gn);
        });

        console.log("--- gn=" + gn);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderGainCB(parseFloat(document.querySelector('#'+sliderId).value));
    };
}

if(document.querySelector('#'+gnId) && document.querySelector('#'+gainId))
    addGain(gainId, gnId, controlNames);

////////////////////////////////////////////////////////////////////////////////////////////////////////

};



/////////////////////////////// modulation/error-correction scheme //////////////////////////////////////



function addModSlider() {


    var md = 1; // starting mode at "r2/3 BPSK"

    var modes = [

        // These must stay consistent with strings like this in
        // ../../share/crts/plugins/Filters/liquidFrame.cpp.

        "r1/2 BPSK      ",
        "r2/3 BPSK      ",
        "r1/2 QPSK      ",
        "r2/3 QPSK      ",
        "r8/9 QPSK      ",
        "r2/3 16-QAM    ",
        "r8/9 16-QAM    ",
        "r8/9 32-QAM    ",
        "r8/9 64-QAM    ",
        "r8/9 128-QAM   ",
        "r8/9 256-QAM   ",
        "uncoded 256-QAM"
    ];

    serverModeToSliderCB = function(md_in) {

        // This should update the mode slider from the web.
        //

        md = md_in;
        document.querySelector('#md').value = modes[md];
        document.querySelector('#mode').value = md;
        console.log("md=" + md + "  " + modes[md]);
    }

    serverModeToSliderCB(md);


    function sliderModeCB(md) {

        document.querySelector('#md').value = modes[md];

        if(sendModeCB) 
            sendModeCB(md);

        console.log("--- md=" + md + "  " + modes[md]);
    }

    document.querySelector('#mode').oninput = function() {
        let val = Math.round(parseFloat(document.querySelector('#mode').value));
        if(md != val)
            sliderModeCB(md = val);
    };
}

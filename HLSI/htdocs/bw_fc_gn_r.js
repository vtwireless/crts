
require("/HLSI/session.js");

//////////////////////////////////////////////////////////////////////////////    
// Pick a scenario: 1 or 2
var scenario = (document.querySelector('#ign'))?2:1;

console.log(" ++++++++++++++++++ running scenario = " + scenario);
switch(scenario) {
    case 1:
        f0 = 915.5e6;
        break;
    case 2:
        f0 = 919.5e6;
        break;
}


// Globals that are used in session.js and this file.

var serverBandwidthToSliderCB = {};
var sendBandwidthCB = {};
var serverFreqToSliderCB = {};
var sendFreqCB = {};
var serverGainToSliderCB = {};
var sendGainCB = {};
var bins = 200; // number of fft points per plot or number of datapoints
var f0;/*center Frequency of spectrum plot*/

function plotSpectrum(y) {

    dataf = d3.range(0,bins-1).map(function(i) { 
            return {"y": 20*Math.log10(y[i]) }
    });
    pathf.datum(dataf).attr("d", linef);
}




onload = function() {

    switch(scenario) {

        case 1:

            session(scenario, 'tx1', f0);
            makeSlidersDisplay('tx1'/*controlName*/, 'bandwidth', 'bw', 'frequency', 'fc', 'gain', 'gn');
            break;

        case 2:

            session(scenario, ['tx2', 'tx2_interferer'], f0);
            makeSlidersDisplay('tx2'/*controlName*/, 'bandwidth', 'bw', 'frequency', 'fc', 'gain', 'gn');
            makeSlidersDisplay('tx2_interferer'/*controlName*/, 'ibandwidth', 'ibw', 'ifrequency', 'ifc', 'igain', 'ign');
            break;
    }
};



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






function makeSlidersDisplay(controlName, bandwidthId, bwId, frequencyId, fcId, gainId, gnId) {


var fc = 0;    // some kind of slider relative frequency
var bw = 0.25; // relative to plot width filter bandwidth
var gn = 31.5; // gain in db


/////////////////////////// BANDWIDTH ////////////////////////////////////


function addBandwidth(sliderId, outputId, name) {

    serverBandwidthToSliderCB[name] = function(bandwidth) {

        // This should update the bandwidth slider from the web.

        // bw is a slider relative scale of bandwidth
        bw = bandwidth/(fs*scale_freq);

        document.querySelector('#'+outputId).value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";
        document.querySelector('#'+sliderId).value = bw;

        console.log("bw=" + bw);
    }

    sliderBandwidthCB();

    function sliderBandwidthCB(band) {

        // bw is a slider relative scale of bandwidth
        if (band!=null) { bw = band; }

        document.querySelector('#'+outputId).value = d3.format(".2f")(bw*fs*scale_freq) + " " + units_freq + "Hz";

        if(sendBandwidthCB[name] !== undefined)
            // Send to the web server.
            sendBandwidthCB[name](bw*fs*scale_freq);

        console.log("--- bw=" + bw);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderBandwidthCB(parseFloat(document.querySelector('#'+sliderId).value));
    };
}

if(document.querySelector('#'+bwId) && document.querySelector('#'+bandwidthId))
    addBandwidth(bandwidthId, bwId, controlName);


///////////////////////////////////////////////////////////////////////////


//////////////////////////// FREQUENCY ////////////////////////////////////


function addFreq(sliderId, outputId, name) {
    
    serverFreqToSliderCB[name]= function(freq) {

        // This should update the frequency slider from the web.
        //
        // fc is a slider relative scale of frequency
        fc = freq/(fs*scale_freq) - f0/fs;
        //
        document.querySelector('#'+outputId).value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";
        document.querySelector('#'+sliderId).value = fc;
        console.log("fc=" + fc);
    }

    sliderFreqCB();

    function sliderFreqCB(fc_in) {

        // fc is a slider relative scale of frequency
        if(fc_in!=null) { fc = fc_in; }

        document.querySelector('#'+outputId).value = d3.format(".2f")((f0+fc*fs)*scale_freq) + " " + units_freq + "Hz";

        if(sendFreqCB[name] !== undefined) 
            // Send freq to the web server.
            sendFreqCB[name]((f0+fc*fs)*scale_freq);

        console.log("--- name=" + name + " fc=" + fc + " sendFreqCB[name]=" + sendFreqCB[name]);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderFreqCB(parseFloat(document.querySelector('#'+sliderId).value));
    };

}

if(document.querySelector('#'+fcId) && document.querySelector('#'+frequencyId))
    addFreq(frequencyId, fcId, controlName);


///////////////////////////////////////////////////////////////////////////


/////////////////////////////// GAIN //////////////////////////////////////


function addGain(sliderId, outputId, name) {

    serverGainToSliderCB[name] = function(gain) {

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


    sliderGainCB();

    function sliderGainCB(gn_in) {

        // gn is a slider relative scale of gain.  Looks like it's also gain
        // in db.
        //
        if(gn_in!=null) { gn = gn_in; }

        document.querySelector('#'+outputId).value = d3.format(".1f")(gn) + " dB";

        if(sendGainCB[name] !== undefined) 
            // Send gain to the web server.
            sendGainCB[name](gn);

        console.log("--- gn=" + gn);
    }

    document.querySelector('#'+sliderId).oninput = function() {
        sliderGainCB(parseFloat(document.querySelector('#'+sliderId).value));
    };
}

if(document.querySelector('#'+gnId) && document.querySelector('#'+gainId))
    addGain(gainId, gnId, controlName);


///////////////////////////////////////////////////////////////////////////


};

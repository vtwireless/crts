require('/d3.v5.min.js');
require('/hlsi.css');
require('/plotDimensions.js');
require('/spectrum/feed.js');


// Plot a parameter as a function of time.
//
//  parameter: may be a parameter object or a function that
//             returns the Y value to plot at any time.
//
//
// options argument:
//
//    opts:
//
//       yMin: minimum Y value in plot area in dB
//       yMax: maximum Y value in plot area in dB
//       yLabel: Y axis plot label
//       parentNode:
//
//
//
//
function SpectrumPlot(io, usrpAddr, opts = {}) {

    function getOpt(x, defaultVal) {
        if(opts[x] !== undefined)
            return opts[x];
        return defaultVal;
    };

    var parentNode   = getOpt('parentNode' , null   );
    var yMin         = getOpt('yMin'       , -100.0 );
    var yMax         = getOpt('yMax'       ,  60.0  );
    var yLabel       = getOpt('yLabel'     , ''     );

    var deltaY = yMax - yMin;


    if(typeof(parentNode) === "string") {
        let str = parentNode;
        // We'll assume that this is a CSS Selector string
        // for a page with HTML already built.
        parentNode = document.querySelector(str);
        if(!parentNode)
            throw("Slider element parent \"" + str + '" was not found');
        if(parentNode.className === undefined)
            parentNode.className = 'scrollPlot';
    } else if(parentNode === null) {
        parentNode = document.createElement("div");
        parentNode.className = 'scrollPlot';
        document.body.appendChild(parentNode);
    } // Else
      // the user passed in a parentNode that we will try to use.

    const width =  plot.width;
    const height = plot.height;
    const margin = plot.margin;


function svg_create(_margin, _width, _height, _xscale, _yscale)
{
    // 1. Add the SVG (time) to the page and employ #2
    var svg = d3.select(parentNode).append("svg")
        .attr("width",  _width  + _margin.left + _margin.right)
        .attr("height", _height + _margin.top +  _margin.bottom)
      .append("g")
        .attr("transform", "translate(" + _margin.left + "," + _margin.top + ")");

    svg.append("rect")
        .attr("width", "86.2%")
	.attr("height", "81%")
	.attr("fill", "black");

    // create axes
    svg.append("g").attr("class", "x axis")
        .attr("transform", "translate(0," + _height + ")")
        .call(d3.axisBottom(_xscale));
    svg.append("g").attr("class", "y axis")
        .call(d3.axisLeft(_yscale));

    // create grid lines
    svg.append("g").attr("class","grid").call(d3.axisBottom(_xscale).tickFormat("").tickSize( _height));
    svg.append("g").attr("class","grid").call(d3.axisLeft  (_yscale).tickFormat("").tickSize(- _width));
    return svg;
}

// determine scale and units for sample value v; use p to adjust cut-off threshold
function scale_units(v,p)
{
    let r = v * (p==null ? 1 : p);
    if      (r > 1e+12) { return [1e-12, 'T']; }
    else if (r > 1e+09) { return [1e-09, 'G']; }
    else if (r > 1e+06) { return [1e-06, 'M']; }
    else if (r > 1e+03) { return [1e-03, 'k']; }
    else if (r > 1e+00) { return [1e+00, '' ]; }
    else if (r > 1e-03) { return [1e+03, 'm']; }
    else if (r > 1e-06) { return [1e+06, 'u']; }
    else if (r > 1e-09) { return [1e+09, 'n']; }
    else if (r > 1e-12) { return [1e+12, 'p']; }
    else                { return [1e+16, 'f']; }
}


    var cFreq, bw = 0.0, updatePeriod, bins;
    var scale_freq;
    var units_freq
    var pathf;
    var linef;

    function makePlot(_cFreq, _bw, _updatePeriod, _bins) {

        if(bw !== 0.0)
            // We call this only once.
            return;

        cFreq = _cFreq;
        bw = _bw;
        updatePeriod = _updatePeriod;
        bins = _bins;

        [scale_freq, units_freq] = scale_units(cFreq + bw/2 , 0.1);

        // 5. X scale will use the index of our data
        var fScale = d3.scaleLinear().domain([(cFreq-0.5*bw)*scale_freq,
                (cFreq+0.5*bw)*scale_freq]).range([0, width]);

        var pScale = d3.scaleLinear().domain([yMin, yMax]).range([height, 0]);

        // 7. d3's line generator
        linef = d3.line()
            .x(function(d, i) { return fScale((cFreq-0.5*bw + i*bw/(bins-1))*scale_freq); })  // map frequency
            .y(function(d)    { return pScale(d.y);        }); // map PSD


        // 8. An array of objects of length N. Each object has key -> value pair,
        // the key being "y" and the value is a random number
        var dataf = d3.range(0,bins).map(function(f) { return {"y": 0 } })

        // create SVG objects
        var svg = svg_create(margin, width, height, fScale, pScale);

        // create x-axis axis label
        svg.append("text")
            .attr("transform","translate("+(width/2)+","+(height + 0.75*margin.bottom)+")")
            .attr("dy","-0.3em")
            .style("text-anchor","middle")
            .attr("fill", "white")
            .text("Frequency ("+units_freq+"Hz)");

        // create y-axis label
        svg.append("text")
            .attr("transform","rotate(-90)")
            .attr("y", 0 - margin.left)
            .attr("x", 0 - (height/2))
            .attr("dy", "1em")
            .style("text-anchor","middle")
            .attr("fill", "white")
            .text("Power Spectral Density (dB)")

        // clip paths
        svg.append("clipPath").attr("id","clipf").append("rect").attr("width",width).attr("height",height);

        // 9. Append the path, bind the data, and call the line generator
        pathf = svg.append("path")
            .attr("clip-path","url(#clipf)")
            .datum(dataf)
            .attr("class", "no-fill")
            .attr("stroke","yellow")
            .attr("stroke-width", 2.0)
            .attr("d", (linef));
    }


    function updatePlot(id, cFreq, bandwidth, updatePeriod, y) {


        dataf = d3.range(0,bins).map(function(i) { 
                return {"y": 10*Math.log10(y[i]) }
                // don't know how, but y is negative numbers, so can't take log
                //return {"y": 20*Math.log10(y[i]) }
        });
        pathf.datum(dataf).attr("d", linef);
    }

    var obj = {
        usrpAddr: usrpAddr,
        makePlot: makePlot,
        updatePlot: updatePlot
    };

    SpectrumPlot.objs.push(obj);


    // spectrumFeeds is a singlton object.  Only the first call to
    // spectrumFeeds() does anything, all other calls do nothing.
    //
    // TODO: make it so we can make a SpectrumPlot() after the
    //       newSpectrum events.

    SpectrumPlot.sp = spectrumFeeds(io, {
        newSpectrum: function(id, description, cFreq,
                bandwidth, updatePeriod, bins) {

            console.log("newSpectrum(" + JSON.stringify(arguments));
            //newSpectrum("0", "peach:addr=192.168.0.107", 914500000,
            //    4000000, 0.1, 80/*bins*/)
            //
            // example: description = "samson:addr=192.168.0.107"
            // We match the 192.168.0.107 part.
            //

            SpectrumPlot.objs.forEach(function(obj) {

                if(description.replace(/.*addr=/, '') === obj.usrpAddr) {
                    obj.makePlot(cFreq, bandwidth, updatePeriod, bins);
                    SpectrumPlot.sp.subscribe(id,  obj.updatePlot);
                }

            });
        }
    });
}


SpectrumPlot.objs = [];

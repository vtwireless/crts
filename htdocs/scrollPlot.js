require('/controllers.js');
require('/d3.v5.min.js');
require('/hlsi.css');
require('/plotDimensions.js');


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
//       yScale: float to multiple the parameter value by
//               to get a number to display
//
//       updatePeriod: float in seconds
//
//       parentNode:
//
//
//
//
function ScrollPlot(parameter, opts = {}) {

    function getOpt(x, defaultVal) {
        if(opts[x] !== undefined)
            return opts[x];
        return defaultVal;
    };


    var updatePeriod = getOpt('updatePeriod', 0.1/*seconds*/ );
    var yScale       = getOpt('yScale'      , 1.0            );
    var parentNode   = getOpt('parentNode'  , null           );
    var yMin         = getOpt('yMin'        , 0.0            );
    var yMax         = getOpt('yMax'        , 1.0            );
    var yTicks       = getOpt('yTicks'      , [ 0.0, 0.25, 0.5, 0.75, 1.0 ]);
    var yLog         = getOpt('yLog'        , false          );
    var yLabel       = getOpt('yLabel'      , ''             );
    var autoScaleY   = getOpt('autoScaleY'  , false          );

    // Default Y value to plot:
    var getValue = function() { return value; };


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



// determine scale and units for sample value v; use p to adjust cut-off threshold
function scale_units(v,p)
{
    if(!autoScaleY) return [ 1.0, '' ];

    let r = v * (p==null ? 1 : p);
    if      (r >= 1e+12) { return [1e-12, 'T']; }
    else if (r >= 1e+09) { return [1e-09, 'G']; }
    else if (r >= 1e+06) { return [1e-06, 'M']; }
    else if (r >= 1e+03) { return [1e-03, 'k']; }
    else if (r >= 1e+00) { return [1e+00, '' ]; }
    else if (r >= 1e-03) { return [1e+03, 'm']; }
    else if (r >= 1e-06) { return [1e+06, 'u']; }
    else if (r >= 1e-09) { return [1e+09, 'n']; }
    else if (r >= 1e-12) { return [1e+12, 'p']; }
    else                 { return [1e+16, 'f']; }
}


    var width =  plot.width;
    var height = plot.height;
    var margin = plot.margin;

    // Time between plotting points:
    var plot_period = updatePeriod; // in seconds

    var interval = setInterval(update_plot, plot_period * 1000);

    // length of x-axis (time) for whole plot:
    var plot_time_length = 90.0; // seconds


    var num_steps = plot_time_length / plot_period;

    var yStart = yMin - 0.3 * deltaY;
    if(yLog && yStart < 0.0)
        // TODO: this is arbitrary.  We just can't have a log of a number
        // less than zero.
        yStart = 1e-10;

    // Y Value plotted for the default case.
    var value = yStart*yScale;

    var datar = d3.range(0,num_steps-1).map(function(f) { return {"y": yStart*yScale} });

    var svgr = d3.select(parentNode)
        .append("svg")
        .attr("width",  width  + margin.left + margin.right)
        .attr("height", height + margin.top +  margin.bottom)
        .append("g")
        .attr("transform", "translate(" +
            margin.left + "," + margin.top + ")");

    var tscale = d3.scaleLinear().domain([-plot_period*(num_steps), 0])
        .range([0, width]);

    if(yLog)
        var rscale = d3.scaleLog().domain([
                yMin, yMax]).range([height, 0]);
    else    
        var rscale = d3.scaleLinear().domain([
                yMin,
                yMax]).range([height, 0]);
     
    var liner = d3.line()
        .x(function(d, i) { return tscale(-plot_period*(num_steps-i)); })
        .y(function(d, i) { return rscale(d.y); });

    svgr.append("defs").append("clipPath")
        .attr("id", "clipr")
        .append("rect")
        .attr("width", width)
        .attr("height", height);

    svgr.append("rect")
	.attr("width", "86.2%")
	.attr("height", "81%")
	.attr("fill", "black");

    svgr.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + height + ")")
        .call(d3.axisBottom(tscale));

    svgr.append("g")
        .attr("class", "y axis")
        .call(d3.axisLeft(rscale)
        .tickValues(yTicks)
        .tickFormat(function(d, i) {
            let [s,u] = scale_units(d);
            return d*s + u;
        }));

    // grid lines
    svgr.append("g")
        .attr("class","grid")
        .call(d3.axisBottom(tscale)
            .tickFormat("").tickSize(height));

    svgr.append("g").attr("class","grid")
        .call(d3.axisLeft(rscale).tickFormat("")
            .tickSize(-width).tickValues(yTicks));

    // create x-axis axis label
    svgr.append("text")
        .attr("transform","translate("+(width/2)+","+
            (height + 0.75*margin.bottom)+")")
        .attr("dy","-0.3em")
        .style("text-anchor","middle")
   	.attr("fill", "white")
        .text("Time (seconds)");


    // create y-axis label
    svgr.append("text")
        .attr("transform","rotate(-90)")
        .attr("y", 0 - margin.left)
        .attr("x", 0 - (height/2))
        .attr("dy", "1em")
        .style("text-anchor","middle")
   	.attr("fill", "white")
        .text(yLabel);

    // 9. Append the path, bind the data, and call the line generator
    var pathr = svgr.append("path")
        .attr("clip-path","url(#clipr)")
        .datum(datar)
        .attr("class", "stroke-med no-fill stroke-orange")
        .attr("d", liner);


    function update_plot() {

        value = getValue();

        // update historical plot
        datar.push({"y": value * yScale});
        datar.shift();
        pathr.datum(datar).attr("d", liner);
    }

    if(typeof(parameter) !== 'function')
        parameter.addOnChange(function(value_in) {
            value = value_in;
        });
    else
        getValue = parameter;
}


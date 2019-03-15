
function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '--------------------------------------------------------';
    text += '\n' + line + '\nCALL STACK\n' + line + '\n' +
        new Error().stack + '\n' + line + '\n';
    console.log(text);

    alert(text);
    window.stop();
    throw "javascript error"
}

function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}


function _GetGridSpacing(pixPerGrid, min, max, pixels/*width or height*/) {

    assert(min < max);
    assert(pixPerGrid >= 1.0);

    let delta = pixPerGrid*(max - min)/pixels;

    let p = Math.floor(Math.log10(delta));
    let upow = Math.pow(10, p);

    let b = delta/upow;

    //console.log('pixels=' + pixels + ' p=' + p +' max - min =' +
    //(max - min) + '  pixPerGrid = ' + pixPerGrid + ' delta=' + delta +
    //' upow=' + upow + '  b=' + b);

    /* round up to b = 1, 2, 5, times 10^p */
    if(b > 10) {
        // Round up to 2 x 10^(p+1)
        // Maybe this case can't happen?
        delta = 20 * upow;
        p += 1;
        upow = Math.pow(10, p);
        b = 2;
    } else if(b > 5) {
        // Round up to 1 x 10^(p+1)
        p += 1;
        delta = upow = Math.pow(10, p);
        b = 1;
    } else if(b > 2) {
        // Round up to 5 x 10^p
        b = 5;
        delta = b * upow;
    } else if(b > 1) {
        // Round up to 2 x 10^p
        b = 2;
        delta = b * upow;
    } else {
        // b == 1 // assume that b < 1
        // Round up to 1 x 10^p
        b = 1
        delta = upow;
    }
    // TODO: We could add grid drawing options that let the use choose
    // between having grids with b= 1,2,5 or 1,5 or 1,2,2.5,5 or just 1 or
    // whatever.


    // The pixels per grid is greater than or equal to the requested
    // pixPerGrid
    let _pixPerGrid = delta*pixels/(max - min);

    let size = Math.max(Math.abs(min),Math.abs(max));

    // TODO: The format of the grid text number labels needs work, it uses
    // digits which is computed here:
    let digits = Math.ceil(Math.log10((size+delta)/delta));
    if(b > 1) ++digits;

    // Now find the start
    //
    // If we do not have the width of the lines and they may vary we add
    // an addition delta behind the min in case the width of the line
    // would make a grid line behind the min show.
    let start = Math.trunc((min-delta)/delta) * delta;

    // debug spew
    //console.log('\n\n pixels=' + pixels + ' min=' + min + 
    //' max=' + max + ' pixPerGrid=' + pixPerGrid + ' delta=' +
    //delta + ' start=' + start);
    //console.log('delta =' + delta + ' ~= ' + b + 'x10^' + p + ' digits=' + digits  );
    //console.log('calculated pixPerGrid=' + (pixels * delta /(max - min)));

    return { 
        start: start,
        delta: delta,
        pixPerGrid: _pixPerGrid,
        digits: digits, // decimal places
        n: b // 1, 2, 5
    };
}


//
// opts = {
//      xMin: float,
//      xMax: float,
//      yMin: float,
//      yMax: float,
//      autoScale: bool
// }
//
function Scope(opts = null) {

    /////////////////////////////// CONFIGURATION /////////////////////////

    // Padding space in pixels around the plot.
    const padWidth = 10;


    // There may be 1, 2, or 3 sets of grid lines depending on the width
    // and height of the canvas.

    var backgroundColor =  'rgb(134, 186, 228)';

    var majGridXWidth = 2.6;
    var majGridYWidth = 2.6;

    var midGridXWidth = 1.2;
    var midGridYWidth = 1.2;

    var minGridXWidth = 0.22;
    var minGridYWidth = 0.22;

    var majGridXColor = 'rgb(8,8,8)';
    var majGridYColor = 'rgb(8,8,8)';

    var midGridXColor = 'rgb(40,40,40)';
    var midGridYColor = 'rgb(40,40,40)';

    var minGridXColor = 'rgb(100,100,100)';
    var minGridYColor = 'rgb(100,100,100)';

    // For the number labels on the major
    let gridFont = '20px serif';
    var xGridFont = gridFont;
    var yGridFont = gridFont;

    // Minimum major Grid pixels per grid line.  Larger means the grid
    // lines have more pixels between them.
    //
    // for major vertical grid lines with number labels
    var majPixPerGridX = 170.0; // yes a float.
    // for major horizontal grid lines with number labels
    var majPixPerGridY = 100.0;


    // for minor vertical grid lines without number labels
    var minPixPerGridX = 18.0;
    // for minor horizontal grid lines without number labels
    var minPixPerGridY = 14.0;

    if(opts === null || opts.autoScale !== false)
        var autoScale = true;
    else
        var autoScale = false;

    // Used to set the current xScale, xShift, yScale, yShift
    // for xPix() and yPix();
    var xMin = null, xMax, yMin, yMax;

    if(autoScale) {

        // Used to dynamically calculate xMin, xMax, yMin, and yMax,
        // because we can't just change them in one frame, we need to
        // change xMin, xMax, yMin, and yMax in a smooth fashion (at least
        // when shrinking the plot) while new* are the current mins and
        // maxs from the data which are set with each frame, for the
        // clear/draw mode, which clears the plot before each frame is
        // drawn.
        var newXMin = null, newXMax, newYMin, newYMax;
        var xMinTrigger = false, xMaxTrigger = false,
            yMinTrigger = false, yMaxTrigger = false;
    }

    // These are the min and max values across the whole canvas.  These
    // min and max values are not from value that are input, but are the
    // limiting values at the edges of the canvas and a little padding.
    if(opts && opts.xMin !== undefined && opts.xMax !== undefined &&
            opts.yMin !== undefined && opts.yMax !== undefined) {
        // We may or may not be using autoScale in this case, but in
        // either case the user passed in starting limit values.
        xMin = opts.xMin;
        xMax = opts.xMax;
        yMin = opts.yMin;
        yMax = opts.yMax;
        assert(xMin < xMax);
        assert(yMin < yMax);
        // In this case the user choose to set the scales.
    } else {
        // In this case we must be using auto scaling.  The values for
        // xMin, xMax, yMin, and yMax may change before each draw cycle.
        // These will get set later.
        assert(autoScale === true,
                "autoScale is not true and xMin, " +
                "xMax, yMin, yMax are not all set")
    }


    // We define a function to calculate the xScale, yScale, xShift, and
    // yShift, that are used in functions xPix() and yPix().  We need this
    // because xMin, xMax, yMin, and yMax change as the canvas
    // automatically scales as data is input into it.  XPix() and
    // yPix(), are called frequently and is a linear function with just 2
    // multiples and 2 adds in each.
    function calculateScales(w, h) {

        let w_1 = w-1;
        let h_1 = h-1;

        xScale =  (w_1 - 2 * padWidth)/(xMax - xMin);
        xShift = xScale * xMin - padWidth;

        yScale = - (h_1 - 2 * padWidth)/(yMax - yMin);
        yShift = yScale * yMax - padWidth;
    }

    // We count the number of frames?  We do not care if this int wraps
    // back to zero, because we on use differences.
    //var frameCount = 0;

    // Dynamically change xMin, xMax, yMin, yMax based on looking at values
    // plotted and this dynamical model.
    //
    // Or more exactly:
    // Change xMin, xMax, yMin, yMax based on looking at newXMin, newXMin,
    // newXMax, newYMin, and newYMax, where newXMin, newXMin, newXMax,
    // newYMin, and newYMax, are gotten from the current X, Y input values
    // that will be plotted.
    //
    // We want this to be a separate function because it could get a
    // little complex, with a dynamical model and the like.
    //
    // Returns true if a redraw of the background grid and the plots is
    // needed.
    //
    function integrateScalingDynamics() {

        // For each scaling degree of freedom (xMin, xMax, yMin, yMax), X,
        // we have an equation of motion (ODE) ordinary differential
        // equation like:
        //
        //    X_dot = - (X- X_target) * t / autoScalePeriod := f(t)
        //
        // and we also add upper and lower thresholds so that when X is
        // between the thresholds we do not change X.
        //
        //
        // That is one ODE for each of xMin, xMax, yMin, yMax.  We just
        // use a simple Euler's method to integrate.  So:
        //
        //   X(t+dt) = X(t) + f(t) * dt  =>  X += f(t) with dt = 1
        //
        //  TODO: Should it be a second order ODE.
        //
        //  dt = 1 so ya, this is approximating continuous time with
        //  decrease time.  We expect the real (wall clock) time that
        //  passes to be about 1/60 seconds between calls to this
        //  function.  If this function is not call regularly, that's
        //  okay, it's just like stopping the clock, and the plot will
        //  just rescale more slowly.
        //
        //  TODO: Add a dynamic reset for when this is posed, so that
        //  we can make the scaling perfect after it is "stopped and
        //  restarted".

        // Advance the dynamic scaling to the next time step or frame, or
        // whatever you want to call it.  This should be faster than
        // computing an exponential, and should be a stable solver.


        /* We illustrate the dynamic canvas boundary X max.  The other
         * boundaries X min, Y min, and Y max are similar, just with
         * independent dynamics, parameters and so on.


          delta = max - min (is required to be greater than zero)



          ------------ max  (for scale purposes)
          |
          |
    ^     |
    |     ------------ mid -----------------
    |     |                                |
    +     ------------ stop                |
          |                                |
          |                                |  
          ------------ lower               |
                                           |
                                           V
                                         force varies 

         ------- current new max



        This sled thing has 4 levels, max, mid, stop, and lower, all fixed
        rigidly to the sled.  The so called "force" is only applied to the
        sled when some threshold conditions are met, and this force only
        pulls the sled down.  The sled only moves down when the force is
        applied.  The sled can move up into a new position in one step,
        but only when another threshold condition is met.  These jerky
        upward motions are avoided by choosing good values for parameters:
        autoScalePeriod, edgeFac, and triggerFac.  The sled does change
        it's size as the x and y, max and min values change.  It's not
        really a force because the dynamics is only a first order ordinary
        differential equation (ODE), but maybe it is if we think of it as
        an over damped dynamical system.  TODO: Maybe we should add a form
        of inertia to the dynamics.


        delta = max - min (is required to be greater than zero, not shown)


        max -> is the current scale variable which can be xMax, xMin, yMax,
              or yMin.
    
        lower = max - delta * edgeFac,

        mid = (max - lower)/2.0,
        
        stop = mid - (mid - lower) * triggerFac,

        
        Some of the signs (-) are changed depending on wither it's a max
        or a min border we are dealing with.


        */

        // We do not define some variables if dynamic "autoScale" is not
        // used.

        // autoScalePeriod is the number of draw cycles for the plot
        // "scale" to shrink due to input values changing their extent,
        // max and min values.  Otherwise with noisy data the plot scale
        // will be jumping in a very distracting fashion.  Some other
        // plotters do not make good auto-scaled plots when data is like
        // "real-world" noisy data.  We damp motion of the scale when it
        // gets smaller and we buffer the scale with a threshold, so we
        // don't jump to different scales until a threshold is crossed.
        //
        // autoScalePeriod is a linear damping period in number of draw
        // cycles.  When the scaling dynamics is "active" the
        // autoScalePeriod is like a exponential damping constant.  The
        // scaling dynamics is "active" when certain thresholds are
        // reached in the current max and min values user plotted X and Y
        // input values.  
        //
        const autoScalePeriod = 20; // autoScalePeriod must be 1 or greater

        // edgeFac is the factional size of the dynamical plot edges, so the
        // scale will not change if the current extent is within deltaFac
        // times max - min.  These are threshold constants that are
        // used to get what factions used with x,y min and max values.
        // edgeFac must be positive and greater than zero.
        // triggerFac must be positive and greater than zero.
        //
        const edgeFac = 0.1, triggerFac = 0.1;


        let deltaX = xMax - xMin, deltaY = yMax - yMin;

        // Each dynamic variable, xMin, xMax, yMin, and yMax evolve
        // independent of each other.

        ///////////////////////////////////////////////////////////////
        // xMax -- Start with checking/changing the xMax
        ///////////////////////////////////////////////////////////////
        //
        let lower = xMax - deltaX * edgeFac;
        let mid = (xMax - lower)/2.0;
        let stop = mid - (mid - lower) * triggerFac;

        // Each dynamic variable, xMin, xMax, yMin, and yMax evolve
        // independent of each other.

        if((newXMax <= xMax && newXMax >= stop) ||
            (!xMaxTrigger && newXMax >= lower && newXMax <= xMax)) {

            if(xMaxTrigger) xMaxTrigger = false;

            // Do nothing

        } else if((xMaxTrigger && newXMax < stop) || newXMax < lower) {

            if(!xMaxTrigger) xMaxTrigger = true;

            // The slow DYNAMICS: pull the xMax down with a simple
            // exponential "decay like" form.  The user still sees all the
            // X,Y data that is plotted, but we slowly zoom in for a
            // better scaling (view) of the data.
            //
            // This actually is like an Euler's method solver.  It just
            // turns out to be able to be put into this very simple form:
            // remember mid is xMax - constant, so this is like
            // exponential decay with asymptote of mid (= newXMax) with
            // the rate being zero if mid = newXMax.  It's a little
            // weirder because newXMax is changing with each frame, so the
            // rate may be discontinuous, but xMax and mid is continuous,
            // like in a digital approximation of a continuous function.
            // For you ODE purists, maybe this is just an iterative map
            // with an external input, newXMax.  At some point it is what
            // it is and it just runs, and behaves nicely if newXMax
            // is not changing to drastically.
            xMax -= (mid - newXMax)/autoScalePeriod;

            // Moving xMax may have pulled xMax past the stop.
            if(xMax >= stop) xMaxTrigger = false;
        } else {

            if(xMax >= stop) xMaxTrigger = false;
            // This is the jerky case.  We move xMax in one frame.
            // We do this because otherwise the user could not see all
            // the data.
            xMax = mid;
        }


    


        return false;

    }
    


    ///////////////////////////////////////////////////////////////////////
    //
    // This class object has 2 canvases, all the same size.
    //
    //   1. render:  the canvas that is showing in the window
    //
    //   2. bg: a canvas (buffer) that we put the constant background on
    //


    var render = document.createElement('canvas');
    render.className = 'render';

    this.getElement = function() {
        return render;
    };


    var ctx = render.getContext('2d');

    var bg = document.createElement('canvas');
    bg.className = 'bg';
    bgCtx = bg.getContext('2d');


    // We define:  pixelX = xScale * X_in - xShift
    //       and:  pixelY = yScale * Y_in - yShift
    //
    // where X_in and Y_in are raw user input values.
    //
    var xScale, yScale, xShift, yShift;


    // Functions to convert from user x (and y) values to pixel coordinates.
    function xPix(x) {
        return (xScale * x - xShift);
    }
    function yPix(y) {
        return (yScale * y - yShift);
    }


    function drawXgrid(gridXWidth, gridXColor, gridX, gridY=null, xGridFont=null) {

        bgCtx.lineWidth = gridXWidth;
        bgCtx.strokeStyle = gridXColor;
        bgCtx.fillStyle = gridXColor;
        if(xGridFont)
            bgCtx.font = xGridFont;

        let n = 0;
        let max = xMax + gridX.delta;

        for(let x = gridX.start; x <= max;
            x = gridX.start + (++n)*gridX.delta) {
            let xpix = xPix(x);
            bgCtx.beginPath();
            bgCtx.moveTo(xpix, 0);
            bgCtx.lineTo(xpix, h);
            bgCtx.stroke();
            if(xGridFont)
                // TODO: The format of the text needs work.
                bgCtx.fillText(x.toPrecision(gridX.digits),
                        xpix+3,
                        yPix(gridY.start + 1*gridY.delta) -
                        gridY.pixPerGrid/2 + 4);
        }
    }

    function drawYgrid(gridYWidth, gridYColor, gridY, gridX=null, yGridFont=null) {

        bgCtx.lineWidth = gridYWidth;
        bgCtx.strokeStyle = gridYColor;
        bgCtx.fillStyle = gridYColor;
        if(yGridFont)
            bgCtx.font = yGridFont;

        let n = 0;
        let max = yMax + gridY.delta;

        for(let y = gridY.start; y <= max;
            y = gridY.start + (++n)*gridY.delta) {
            let ypix = yPix(y);
            bgCtx.beginPath();
            bgCtx.moveTo(0, ypix);
            bgCtx.lineTo(w, ypix);
            bgCtx.stroke();
            if(yGridFont)
                // TODO: The format of the text needs work.
                bgCtx.fillText(y.toPrecision(gridY.digits),
                        xPix(gridX.start + 1.5*gridX.delta) - 10,
                        ypix-4);
        }
    }


    function drawBackground() {

        // Draw the bg grid from the background canvas, bg, to the
        // rendered canvas.
        ctx.clearRect(0, 0, w, h);
        ctx.drawImage(bg, 0, 0);
    }


    function recreateBackgroundCanvas() {

        w = render.width = render.offsetWidth;
        h = render.height = render.offsetHeight;
        bg.width = w;
        bg.height = h;

        calculateScales(w, h);

        function remainder(x) { return Math.abs(x - Math.round(x)); }

        // TODO: We can use clearRect() to make the background transparent
        //bgCtx.clearRect(0, 0, w, h);
        bgCtx.fillStyle = backgroundColor;
        bgCtx.rect(0, 0, w, h);
        bgCtx.fill();


        let majGridX = _GetGridSpacing(majPixPerGridX, xMin, xMax, w/*pixels*/);
        let majGridY = _GetGridSpacing(majPixPerGridY, yMin, yMax, h/*pixels*/);

        // These computed grids are okay, but do not necessarily align
        // with the major grids, so below we keep testing them until they
        // align with the major grid or we'll skip drawing them.
        let minGridX = _GetGridSpacing(minPixPerGridX, xMin, xMax, w/*pixels*/);
        let minGridY = _GetGridSpacing(minPixPerGridY, yMin, yMax, h/*pixels*/);

        while(remainder(majGridX.pixPerGrid/minGridX.pixPerGrid) > 0.1 &&
                minGridX.pixPerGrid < majGridX.pixPerGrid)
            // The major and minor grid lines do not line up, so go to the next size.
            minGridX = _GetGridSpacing(minGridX.pixPerGrid + 1.0, xMin, xMax, w/*pixels*/);
        while(remainder(majGridY.pixPerGrid/minGridY.pixPerGrid) > 0.1 &&
                minGridX.pixPerGrid < majGridX.pixPerGrid)
            // The major and minor grid lines do not line up, so go to the next size.
            minGridY = _GetGridSpacing(minGridY.pixPerGrid + 1.0, yMin, yMax, h/*pixels*/);

        if(minGridX.pixPerGrid < majGridX.pixPerGrid)
            drawXgrid(minGridXWidth, minGridXColor, minGridX);
        if(minGridX.pixPerGrid < majGridX.pixPerGrid)
            drawYgrid(minGridYWidth, minGridYColor, minGridY);

        // We draw a 3rd X grid if we can.  This grid is between the size
        // of the minor grid and the major grid.
        if(minGridX.pixPerGrid < majGridX.pixPerGrid) {
            let midGridX = _GetGridSpacing(minGridX.pixPerGrid + 0.1, xMin, xMax, w/*pixels*/);
            while((remainder(majGridX.pixPerGrid/midGridX.pixPerGrid) > 0.1 ||
                    remainder(midGridX.pixPerGrid/minGridX.pixPerGrid) > 0.1) &&
                midGridX.pixPerGrid < majGridX.pixPerGrid)
                // The two grid lines do not line up, so go to the next size.
                midGridX = _GetGridSpacing(midGridX.pixPerGrid + 0.1, xMin, xMax, w/*pixels*/);
            //console.log("midGridX.pixPerGrid=" + midGridX.pixPerGrid + ' majGridX.pixPerGrid=' +
            //    majGridX.pixPerGrid + ' midGridX.delta=' + midGridX.delta );
            if(midGridX.pixPerGrid < majGridX.pixPerGrid) {
                drawXgrid(midGridXWidth, midGridXColor, midGridX);
            }
        }

        // We draw a 3rd Y grid if we can.  This grid is between the size
        // of the minor grid and the major grid.
        if(minGridY.pixPerGrid < majGridY.pixPerGrid) {
            let midGridY = _GetGridSpacing(minGridY.pixPerGrid + 0.1, yMin, yMax, h/*pixels*/);
            while((remainder(majGridY.pixPerGrid/midGridY.pixPerGrid) > 0.1 ||
                    remainder(midGridY.pixPerGrid/minGridY.pixPerGrid) > 0.1) &&
                midGridY.pixPerGrid < majGridY.pixPerGrid)
                // The two grid lines do not line up, so go to the next size.
                midGridY = _GetGridSpacing(midGridY.pixPerGrid + 0.1, yMin, yMax, h/*pixels*/);
            if(midGridY.pixPerGrid < majGridY.pixPerGrid) {
                drawYgrid(midGridYWidth, midGridYColor, midGridY);
            }
        }

        // The major grid gets drawn on the top.
        drawXgrid(majGridXWidth, majGridXColor, majGridX, majGridY, yGridFont);
        drawYgrid(majGridYWidth, majGridYColor, majGridY, majGridX, yGridFont);


        // Do not worry, this bg (background canvas) will get transferred
        // to the main render canvas some time after this function.

     }




    var plots = {};
    var plotCreateCount = 0;

    function Plot(id = -1) {

        if(id === -1)
            this.id = createCount++;
        else
            this.id = id;

        // Add this to the list of plots in this scope.
        plots[this.id] = this;

        // TODO: add plot user optional parameters for sizes and colors.
        var pointDiameter = 5.2;
        var lineWidth = 4.3;
        var lineColor = 'blue';
        var pointColor = 'red';

        // Last x and y values plotted.
        var x = null;
        var y;


        // We separate the data update and the draw function so that
        // we may draw() when there is no new data to draw, like in the
        // case of a App window/canvas (thingy) resize event.
        this.updateData = function(x_in, y_in) {

            x = x_in;
            y = y_in;

            assert(x.length > 0);

            if(autoScale) {
                // Get the current x,y max and min values.
                let l = x.length;
                newXMin = newXMax = x[0];
                newYMin = newYMax = y[0];
                for(let i=1; i<l; ++i) {

                    const smallFloat = 1.0e-25;

                    // Ya.  This is a computer resource eating loop, limit
                    // what you do in here.
                    //
                    // We get the new min and max values to late to use in
                    // this draw frame.
                    //
                    // TODO: we need to consider getting these new min/max
                    // values in the loops that we draw in so that we can
                    // have one less tight loop.
                    if(x[i] < newXMin) newXMin = x[i];
                    else if(x[i] > newXMax) newXMax = x[i];
                    if(y[i] < newYMin) newYMin = y[i];
                    else if(y[i] > newYMax) newYMax = y[i];

                    // We cannot have newXMin == newXMax or newYMin == newYMax
                    // That would give division by zero.  It not fudging
                    // anything, it's just changing the scales.
                    if(newXMin + smallFloat >= newXMax)
                        newXMin = newXMax - smallFloat;
                    if(newYMin + smallFloat >= newYMax)
                        newYMin = newYMax - smallFloat;
                }

                if(xMin === null) {
                    // This is the first call to this so we add a little
                    // more edge pixel padding.  This may keep the auto
                    // scaling from starting with a giggle.
                    //
                    // TODO: We may need to consider numbers in
                    // integrateScalingDynamics() to get better
                    // initialization here.
                    //
                    const fac = 0.0005;
                    let delta = (newXMax - newXMin) * fac;
                    xMin = newXMin - delta;
                    xMax = newXMax + delta;
                    delta = (newYMax - newYMin) * fac;
                    yMin = newYMin - delta;
                    yMax = newYMax + delta;
                }

                // Based on these values (above) compute the auto scaling
                // dynamics which may change the current xMin, xMax, yMin,
                // and yMax values, which the xPix(), and yPix() functions
                // depend on indirectly through the xScale, xShift,
                // yScale, and yShift.
                //
                return integrateScalingDynamics();
            }
            return false; // false = do not redraw because of this code.
        }


        function drawPoint(x, y) {

            ctx.beginPath();
            ctx.arc(xPix(x), yPix(y), pointDiameter/2, 0, Math.PI * 2, true);
            ctx.closePath();
            ctx.fill();
        }

        this.draw = function() {

            assert(x);

            ctx.beginPath();
            ctx.moveTo(xPix(x[0]), yPix(y[0]));
            ctx.strokeStyle = lineColor;
            ctx.lineWidth = lineWidth;
            let l = x.length;
            for(let i=1; i<l; ++i)
                ctx.lineTo(xPix(x[i]), yPix(y[i]));
            ctx.stroke();

            ctx.fillStyle = pointColor;
            // We must draw points after lines if we wish to see them.
            for(let i=0; i<l; ++i)
                drawPoint(x[i], y[i]);
        }

    }



    // You may have many plots on a single graph (or scope).
    // This is the user interface used to make plots.
    this.makePlot = function() {

        if(arguments.length < 1)
            var id = plotCreateCount++;
        else
            var id = arguments[0];

        return (new Plot(id)).id;
    }


    // Initialize these drawing flags.
    var needReplot = true;
    var needNewBackground = true;


    // This is the user interface to plot, and will automatically make
    // a plot if one does not exist with the plotId.
    //
    // This function is called with 2 modes:
    //
    //    1.  With no arguments to make the app redisplay (draw) if it
    //        needs to.
    //
    //    2.  With arguments x, y to change a plot.
    //
    function draw(x, y, plotId = 0) {


        if(plots[plotId] !== undefined)
            var plot = plots[plotId];
        else
            var plot = new Plot(plotId);

        // (at the time of this writing 2019, Mar 14) Because there is no
        // "redraw" or "resize" event in browser javaScript sometimes when
        // this function is called this function will do nothing.  We
        // don't draw unless we need to draw.  The mozilla tutorials do
        // not seem to care about minimising the use of system resource,
        // and so they draw even when is no pixel color change.  For many
        // apps pixels do not change until there is a canvas resize.


        // TODO: fix for multiple plots, so draw is called with different
        // plotId.

        if(arguments.length >= 2) {

            if(plot.updateData(x, y))
                needNewBackground = true;
            needReplot = true;
        }

        assert(xMin !== null);

        if(render.offsetWidth === 0 || render.offsetHeight === 0)
            // It must be iconified, or hidden or like thing so we can't
            // draw anything.
            return;


        if(render.offsetWidth !== render.width ||
                render.offsetHeight !== render.height || needNewBackground) {
            // We do not draw (resize()) if there is no change
            // in what we would draw.
            recreateBackgroundCanvas();
            needReplot = true;
        }

        if(needReplot) {
            drawBackground();
            // TODO: Add more plots.  How do we know which plot to draw?
            plot.draw();
            needReplot = false;
        }
    }

    this.draw = draw;
}

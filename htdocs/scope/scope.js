
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

    let b = Math.ceil(delta/upow);

    /* round up to b = 1, 2, 5, times 10^p */
    if(b > 10) {
        // Maybe this case can't happen?
        delta = 20 * upow;
        p += 1;
        upow = Math.pow(10, p);
        b = 2;
    } else if(b > 5) {
        p += 1;
        delta = upow = Math.pow(10, p);
        b = 1;
    } else if(b > 2) {
        delta = (b = 5) * upow;
    } else if(b > 1) {
        delta = (b = 2) * upow;
    } else {
        delta = upow;
        // b = 1
    }

    // The pixels per grid is greater than or equal to the requested
    // pixPerGrid
    let _pixPerGrid = delta*pixels/(max - min);

    let size = Math.max(Math.abs(min),Math.abs(max));

    let digits = Math.ceil(Math.log10(size/delta));
    if(b > 1) ++digits;
    if(digits === 0) digits = 1;

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
    console.log('delta =' + delta + ' ~= ' + b + 'x10^' + p + ' digits=' + digits  );
    //console.log('calculated pixPerGrid=' + (pixels * delta /(max - min)));

    return { 
        start: start,
        delta: delta,
        pixPerGrid: _pixPerGrid,
        digits: digits
    };
}




// @param period time to scroll the whole visual canvas.

function Scope(period) {

    /////////////////////////////// CONFIGURATION /////////////////////////
    var majGridXWidth = 2.2;
    var majGridYWidth = 2.2;
    var majGridXColor = 'rgb(8,8,8)';
    var majGridYColor = 'rgb(8,8,8)';
    var gridFont = '20px serif';
    ///////////////////////////////////////////////////////////////////////

    // This class object has 3 canvases, all the same size.
    //
    //  1. render:  the canvas that is showing in the window
    //
    //  2. bg: a canvas (buffer) that we put the constant background on
    //
    //  3. fg: a canvas (buffer) that we rotate through drawing the plot
    //

    var render = document.createElement('canvas');
    render.className = 'render';

    this.getElement = function() {
        return render;
    };
    
    // These are the min and max values across the whole canvas.  These
    // min and max values are not from value that are input, but are the
    // limiting values at the edges of the canvas.
    var xMin = -100.52, xMax = -100.1;
    var yMin = -0.32, yMax = 1.0;

    assert(xMin < xMax);
    assert(yMin < yMax);


    var ctx = render.getContext('2d');

    var bg = document.createElement('canvas');
    bg.className = 'bg';
    bgCtx = bg.getContext('2d');

    var fg = document.createElement('canvas');
    fg.className = 'fg';
    fgCtx = fg.getContext('2d');

    var oldW = -1, oldH = -1, oldSec = -1;

    // Minimum major Grid pixels per grid line.  Larger means the grid
    // lines have more pixels between them.
    var majPixPerGrid = 100;
    var majPixPerGridX = majPixPerGrid; // for vertical grid lines
    var majPixPerGridY = majPixPerGrid; // for horizontal grid lines


    // We define: pixelX = xScale * X_in - xShift
    // and:       pixelY = yScale * Y_in - yShift
    //
    var xScale, yScale, xShift, yShift;

    //////////////////////// 
    var majGridX = null;
    var majGridY = null;

    bgCtx.font = gridFont;


    // Functions to convert from user x values to pixel coordinates.
    function xPix(x) {
        return (xScale * x - xShift);
    }
    function yPix(y) {
        return (yScale * y - yShift);
    }

    function resize() {

        w = render.width = render.offsetWidth;
        h = render.height = render.offsetHeight;
        fg.width = bg.width = w;
        fg.height = bg.height = h;

        majGridX = _GetGridSpacing(majPixPerGridX, xMin, xMax, w/*pixels*/);
        majGridY = _GetGridSpacing(majPixPerGridY, yMin, yMax, h/*pixels*/);

        let w_1 = w-1;
        let h_1 = h-1;

        xScale =  w_1/(xMax - xMin);
        xShift = w_1*xMin/(xMax - xMin);

        yScale = -h_1/(yMax - yMin);
        yShift = -h_1*yMax/(yMax - yMin);


        bgCtx.lineWidth = majGridXWidth;
        bgCtx.strokeStyle = majGridXColor;
        bg.fillStyle = majGridXColor;
        bgCtx.font = gridFont;
        bg.fillStyle = majGridXColor;

        let n = 0;
        let max = xMax + majGridX.delta;

        for(let x = majGridX.start; x <= max;
            x = majGridX.start + (++n)*majGridX.delta) {
            let xpix = xPix(x);
            bgCtx.beginPath();
            bgCtx.moveTo(xpix, 0);
            bgCtx.lineTo(xpix, h);
            bgCtx.stroke();
            bgCtx.fillText(x.toPrecision(majGridX.digits),
                    xpix+3,
                    yPix(majGridY.start + 1*majGridY.delta) -
                    majGridY.pixPerGrid/2 + 4);


        }

        bgCtx.lineWidth = majGridYWidth;
        bgCtx.strokeStyle = majGridYColor;
        bg.fillStyle = majGridYColor;

        n = 0;
        max = yMax + majGridY.delta;

        for(let y = majGridY.start; y <= max;
            y = majGridY.start + (++n)*majGridY.delta) {
            let ypix = yPix(y);
            bgCtx.beginPath();
            bgCtx.moveTo(0, ypix);
            bgCtx.lineTo(w, ypix);
            bgCtx.stroke();
            bgCtx.fillText(y.toPrecision(majGridY.digits),
                    xPix(majGridX.start + 1.5*majGridX.delta) - 10,
                    ypix-4);
        }
     }

    function draw() {

        // Draw the bg grid to the rendered canvas.
        ctx.drawImage(bg, 0, 0);

    }

    function run() {

        var w = render.width;
        var h = render.height;

        //console.log('w= ' + w + '  render.offsetWidth=' + render.offsetWidth);

        if(render.offsetWidth === w && render.offsetHeight === h &&
                majGridX !== null)
            // Nothing new to draw.  We do not draw if there is no change
            // in what we would draw.
            return;
        if(render.offsetWidth === 0 || render.offsetHeight === 0)
            // It must be iconified, or hidden or like thing.
            return;

        resize();
        draw();
 
        //ctx.drawImage(fg, 0, 0);
    }

    this.run = run;
}

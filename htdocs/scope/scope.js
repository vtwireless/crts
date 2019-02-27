
function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '--------------------------------------------------------';
    text += '\n' + line + '\nCALL STACK\n' + line + '\n' +
        new Error().stack + '\n' + line + '\n';
    console.log(text);

    if(crts) crts.cleanup();

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


    // This is easiest thing one could do, but if max and min are not
    // nice round values, this is shit.
    //
    var deltaValue = pixPerGrid*(max - min)/pixels;

    return { start: min, delta: deltaValue };
}




// @param period time to scroll the whole visual canvas.

function Scope(period) {


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
    var xMin = - period, xMax = 0.0;
    var yMin = -0.1, yMax = 1.0;

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
    var majPixPerGrid = 80;
    var majPixPerGridX = majPixPerGrid; // for vertical grid lines
    var majPixPerGridY = majPixPerGrid; // for horizontal grid lines


    // We define: pixelX = xScale * X_in - xShift
    // and:       pixelY = yScale * Y_in - yShift
    //
    var xScale, yScale, xShift, yShift;

    var majGridX = null;
    var majGridY = null;
    var majGridXWidth = 20;
    var majGridYWidth = 2;
    var majGridXColor = 'rgb(16,102,156)';
    var majGridYColor = 'rgb(76,60,178)';

    // Functions to convert from user x values to pixel coordinates.
    function xPix(x) {
        return xScale * x - xShift;
    }
    function yPix(y) {
        return yScale * y - yShift;
    }

    function resize() {

        w = render.width = render.offsetWidth;
        h = render.height = render.offsetHeight;
        fg.width = bg.width = w;
        fg.height = bg.height = h;

        majGridX = _GetGridSpacing(majPixPerGridX, xMin, xMax, w/*pixels*/);
        majGridY = _GetGridSpacing(majPixPerGridY, yMin, yMax, h/*pixels*/);

        xScale =  (w-1)/(xMax - xMin);
        yScale = -(h-1)/(yMax - yMin);
        xShift = (w-1)*xMin/(xMax - xMin);
        yShift = -(h-1)*yMax/(yMax - yMin);

        let n = 0;

        bgCtx.lineWidth = majGridXWidth;
        bgCtx.strokeStyle = majGridXColor;
        let max = xPix(xMax) + majGridXWidth/2 + 1;
        for(let x = xPix(majGridX.start); x <= max;
            x = xPix(majGridX.start + (++n)*majGridX.delta)) {
            bgCtx.beginPath();
            bgCtx.moveTo(x, 0);
            bgCtx.lineTo(x, h);
            bgCtx.stroke();      
        }
        bgCtx.lineWidth = majGridYWidth;
        bgCtx.strokeStyle = majGridYColor;
        n = 0;
        let min = yPix(yMax) - majGridYWidth/2 + 1;
        for(let y = yPix(majGridY.start); y >= min;
            y = yPix(majGridY.start + (++n)*majGridY.delta)) {
            bgCtx.beginPath();
            bgCtx.moveTo(0, y);
            bgCtx.lineTo(w, y);
            bgCtx.stroke();      
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

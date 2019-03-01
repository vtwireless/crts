
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

var count = 0;

function _GetGridSpacing(pixPerGrid, min, max, pixels/*width or height*/) {

    assert(min < max);
    assert(pixPerGrid >= 1.0);

    let delta = pixPerGrid*(max - min)/pixels;

    let p = Math.floor(Math.log10(delta));
    let upow = Math.pow(10, p);

    let b = delta/upow;

    //console.log('pixels=' + pixels + ' p=' + p +' max - min =' + (max - min) + '  pixPerGrid = ' + pixPerGrid + ' delta=' + delta +
      //  ' upow=' + upow + '  b=' + b);

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
        delta = upow;
        // b = 1
    }

    // The pixels per grid is greater than or equal to the requested
    // pixPerGrid
    let _pixPerGrid = delta*pixels/(max - min);

    let size = Math.max(Math.abs(min),Math.abs(max));

    // TODO: The format of the grid text number labels needs work, it uses
    // digits which is comuputed here:
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
        digits: digits
    };
}




// @param period time to scroll the whole visual canvas.

function Scope(period) {

    /////////////////////////////// CONFIGURATION /////////////////////////
    var majGridXWidth = 2.4;
    var majGridYWidth = 2.4;

    var midGridXWidth = 2.1;
    var midGridYWidth = 2.1;

    var minGridXWidth = 0.4;
    var minGridYWidth = 0.4;

    var majGridXColor = 'rgb(8,8,8)';
    var majGridYColor = 'rgb(8,8,8)';

    var midGridXColor = 'rgb(140,40,40)';
    var midGridYColor = 'rgb(140,40,40)';

    var minGridXColor = 'rgb(80,80,80)';
    var minGridYColor = 'rgb(80,80,80)';

    let gridFont = '20px serif';
    var xGridFont = gridFont;
    var yGridFont = gridFont;

    // Minimum major Grid pixels per grid line.  Larger means the grid
    // lines have more pixels between them.
    //
    // for major vertical grid lines with number labels
    var majPixPerGridX = 170.0;
    // for major horizontal grid lines with number labels
    var majPixPerGridY = 100.0;


    // for minor vertical grid lines without number labels
    var minPixPerGridX = 18.0;
    // for minor horizontal grid lines without number labels
    var minPixPerGridY = 14.0;

    // These are the min and max values across the whole canvas.  These
    // min and max values are not from value that are input, but are the
    // limiting values at the edges of the canvas.
    var xMin = -60.2, xMax = 0.01;
    var yMin = -0.32, yMax = 1.0;

    ///////////////////////////////////////////////////////////////////////

    // This class object has 3 canvases, all the same size.
    //
    //  1. render:  the canvas that is showing in the window
    //
    //  2. bg: a canvas (buffer) that we put the constant background on
    //
    //  3. fg: a canvas (buffer) that we rotate through drawing the plot
    //

    assert(xMin < xMax);
    assert(yMin < yMax);


    var render = document.createElement('canvas');
    render.className = 'render';

    this.getElement = function() {
        return render;
    };


    var ctx = render.getContext('2d');

    var bg = document.createElement('canvas');
    bg.className = 'bg';
    bgCtx = bg.getContext('2d');

    var fg = document.createElement('canvas');
    fg.className = 'fg';
    fgCtx = fg.getContext('2d');


    // We define: pixelX = xScale * X_in - xShift
    // and:       pixelY = yScale * Y_in - yShift
    //
    var xScale, yScale, xShift, yShift;


    // Functions to convert from user x values to pixel coordinates.
    function xPix(x) {
        return (xScale * x - xShift);
    }

    function yPix(y) {
        return (yScale * y - yShift);
    }

    function drawXgrid(gridXWidth, gridXColor, gridX, gridY=null, xGridFont=null) {

        bgCtx.lineWidth = gridXWidth;
        bgCtx.strokeStyle = gridXColor;
        bg.fillStyle = gridXColor;
        bg.fillStyle = gridXColor;
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
        bg.fillStyle = gridYColor;
        bg.fillStyle = gridYColor;
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


    function resize() {

        w = render.width = render.offsetWidth;
        h = render.height = render.offsetHeight;
        fg.width = bg.width = w;
        fg.height = bg.height = h;

        let w_1 = w-1;
        let h_1 = h-1;

        xScale =  w_1/(xMax - xMin);
        xShift = w_1*xMin/(xMax - xMin);

        yScale = -h_1/(yMax - yMin);
        yShift = -h_1*yMax/(yMax - yMin);

function dprint(pre, grid) {

    console.log(pre + " delta=" + grid.delta + " pixPerGrid=" + grid.pixPerGrid +
            ' start=' + grid.start);
}

        function remainder(x) { return Math.abs(x - Math.round(x)); }

        let majGridX = _GetGridSpacing(majPixPerGridX, xMin, xMax, w/*pixels*/);
        let majGridY = _GetGridSpacing(majPixPerGridY, yMin, yMax, h/*pixels*/);

        // These computed grids are okay, but do not necessarily align
        // with the major grids, so below we keep testing them until they
        // align with the major grid or we'll skip drawing them.
        let minGridX = _GetGridSpacing(minPixPerGridX, xMin, xMax, w/*pixels*/);
        let minGridY = _GetGridSpacing(minPixPerGridY, yMin, yMax, h/*pixels*/);

        while(remainder(majGridX.pixPerGrid/minGridX.pixPerGrid) > 0.1 &&
                minGridX.pixPerGrid < majGridX.pixPerGrid)
            // The two grid lines do not line up, so go to the next size.
            minGridX = _GetGridSpacing(minGridX.pixPerGrid + 1.0, xMin, xMax, w/*pixels*/);
        while(remainder(majGridY.pixPerGrid/minGridY.pixPerGrid) > 0.1 &&
                minGridX.pixPerGrid < majGridX.pixPerGrid)
            // The two grid lines do not line up, so go to the next size.
            minGridY = _GetGridSpacing(minGridY.pixPerGrid + 1.0, yMin, yMax, h/*pixels*/);

        if(minGridX.pixPerGrid < majGridX.pixPerGrid)
            drawXgrid(minGridXWidth, minGridXColor, minGridX);
        if(minGridX.pixPerGrid < majGridX.pixPerGrid)
            drawYgrid(minGridYWidth, minGridYColor, minGridY);

        // We draw a 3rd X grid if we can.  This grid is between the size
        // of the min grid and the major grid.
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
        // of the min grid and the major grid.
        if(minGridY.pixPerGrid < majGridY.pixPerGrid) {
            let midGridY = _GetGridSpacing(minGridY.pixPerGrid + 0.1, yMin, yMax, h/*pixels*/);
            while((remainder(majGridY.pixPerGrid/midGridY.pixPerGrid) > 0.1 ||
                    remainder(midGridY.pixPerGrid/minGridY.pixPerGrid) > 0.1) &&
                midGridY.pixPerGrid < majGridY.pixPerGrid)
                // The two grid lines do not line up, so go to the next size.
                midGridY = _GetGridSpacing(midGridY.pixPerGrid + 0.1, yMin, yMax, h/*pixels*/);
            if(midGridY.pixPerGrid < majGridY.pixPerGrid)
                drawYgrid(midGridYWidth, midGridYColor, midGridY);
        }


        drawXgrid(majGridXWidth, majGridXColor, majGridX, majGridY, yGridFont);
        drawYgrid(majGridYWidth, majGridYColor, majGridY, majGridX, yGridFont);
     }


    function draw() {

        // Draw the bg grid to the rendered canvas.
        ctx.drawImage(bg, 0, 0);

    }

    var hasRun = false;

    function run() {

        var w = render.width;
        var h = render.height;

        //console.log('w= ' + w + '  render.offsetWidth=' + render.offsetWidth);

        if(render.offsetWidth === w && render.offsetHeight === h && hasRun)
            // Nothing new to draw.  We do not draw if there is no change
            // in what we would draw.
            return;
        if(render.offsetWidth === 0 || render.offsetHeight === 0)
            // It must be iconified, or hidden or like thing.
            return;

        resize();
        draw();

        if(!hasRun) hasRun = true;
 
        //ctx.drawImage(fg, 0, 0);
    }

    this.run = run;
}

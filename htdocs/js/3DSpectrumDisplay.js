// This javaScript needs the crts.js, x3dom javaScript, x3dom.js, and the
// crts.js loaded.


// A list of ThreeDSpectrumDisplay thingys.
var threeDSpectrumDisplay_obj = {};


// Create a x3DOM 3D canvas and use SpectrumDisplay_create() to set
// callbacks to feed it data.
function ThreeDSpectrumDisplay(feedTag, parentNode=null, nSteps = 6) {

    if(threeDSpectrumDisplay_obj[feedTag] !== undefined) {

        // We are calling this more than once for this feedTag.

        var obj = threeDSpectrumDisplay_obj[feedTag];

        if(obj.nSteps !== nSteps) {
            obj.nSteps = nSteps;
            obj.threeDSpectrumDisplay_set(null/*freq*/);
        }

        // We make just one 3DSpectrumDisplay per feedTag.
    }

    if(Object.keys(threeDSpectrumDisplay_obj).length) {

        // TODO:
        // x3dom bugs stopped us from have more than one obj.
        //
        spew('Cannot make more than one ThreeDSpectrumDisplay');
        return;
    }


    // This is the first time calling this for this feedTag.

    var obj = threeDSpectrumDisplay_obj[feedTag] = {};

    obj.nSteps = nSteps; // number of values to display at a time


    function OnLoad() {

        // We can't set elevationGrid and color until
        // 3DSpectrumDisplay.htm is loaded

        // x3dom elevationGrid that is in 3DSpectrumDisplay.htm
        obj.elevationGrid = GetElementById('elevationGrid');

        // color in the x3dom elevationGrid
        obj.color = GetElementById('color');


        function setHeightAndColor() {

            let heights = '';
            let colors = '';
            // The array size is: obj.values[obj.nSteps][obj.bins]
            for(z=0;z<obj.nSteps; ++z) {
                for(let x=0; x<obj.bins; ++x) {
                    heights += obj.values[z][x].toString() + ' ';
                    colors += '0.2 1 0.2   ';
                }
            }
            // Add a zero height row to the front.
            for(let x=0; x<obj.bins; ++x) {
                heights += '0 ';
                colors += '1 0.2 0.2 ';
            }

            obj.elevationGrid.setAttribute('height', heights);
            obj.color.setAttribute('color', colors);

            //spew('height=' + heights);

            //spew('elevationGrid.outerHTML=' + elevationGrid.outerHTML);

            // Reload the x3dom elevationGrid to force it to render
            // our changes.
            let container = elevationGrid.parentNode;

            assert(container, "container is false");

            // This is super stupid.  What a waste.  We reload the
            // elevationGrid in order to get it to render.
            //
            // TODO: tell X3dom to rerender the elevationGrid without
            // reloading it.

	    var content = container.innerHTML;
	    container.innerHTML = content;

            // x3dom elevationGrid that is in 3DSpectrumDisplay.htm
            obj.elevationGrid = GetElementById('elevationGrid');

            // color in the x3dom elevationGrid
            obj.color = GetElementById('color');
        }


        // This is called when parameters change.
        function threeDSpectrumDisplay_set(freq, bandwidth, bins, updateRate) {

            if(freq) {

                // We got new parameters.
                obj.freq = freq;
                obj.bandwidth = bandwidth;
                obj.bins = bins;
                obj.updateRate = updateRate;
            }

            dspew('threeDSpectrumDisplay_set(' +
                'freq=' + freq +
                ', bandwidth=' + bandwidth +
                ', bins=' + bins +
                ', updateRate=' + updateRate +
                ')');

            obj.values = []; // values[nStep][bins]
            // Example: values[2][20]  row 2   bin slot 20

            for(let z=0; z<obj.nSteps; ++z) {
                obj.values[z] = [];
                for(let x=0; x<obj.bins; ++x)
                    obj.values[z][x] = 0.5;
            }

            elevationGrid.setAttribute('xDimension', obj.bins.toString());
            elevationGrid.setAttribute('xspacing', 3.0/(obj.bins));

            elevationGrid.setAttribute('zDimension', (obj.nSteps + 1).toString());
            elevationGrid.setAttribute('zspacing', 3.0/(obj.nSteps + 1));

            setHeightAndColor();
        };

        obj.threeDSpectrumDisplay_set = threeDSpectrumDisplay_set;




        function threeDSpectrumDisplay_update(values) {

            const yScale = 1.0e+3;

            //spew('threeDSpectrumDisplay_update(' + values + ')');

            assert(values.length == obj.bins,
                "threeDSpectrumDisplay_update([" + obj.values +
                "]) with bins=" + obj.bins);

            for(let z=0; z<(obj.nSteps-1); ++z)
                for(let x=0; x<obj.bins; ++x)
                    obj.values[z][x] = obj.values[z+1][x];

            for(let x=0; x<obj.bins; ++x)
                obj.values[obj.nSteps-1][x] = values[x] * yScale;

            setHeightAndColor();
        }

        function threeDSpectrumDisplay_destroy() {

            spew('threeDSpectrumDisplay_destroy()');
            got_threeDSpectrumDisplay = false;
        }

        SpectrumDisplay_create(feedTag,
            threeDSpectrumDisplay_update,
            threeDSpectrumDisplay_set,
            threeDSpectrumDisplay_destroy);

        spew("3DSpectrumDisplay is OKAY");
    }


    if(parentNode === null) {

        // We must have the x3d html loaded already
        // so we proceed to hook it up.
        OnLoad();

    } else {

        // We have a request to load a new <x3d> and then
        // hook it up with the callbacks.
        let xhttp = new XMLHttpRequest();
        let url = '../3DSpectrumDisplay.htm';
        xhttp.open("GET", url);
        xhttp.onreadystatechange = function() {
            
            if(this.readyState == 4 && this.status == 200) {

                let x3d = document.createElement('x3d');
                x3d.innerHTML = this.responseText;
                parentNode.appendChild(x3d);
                if(typeof(x3dom) === 'undefined') {

                    // TODO: x3dom.js cannot be loaded dynamically in
                    // javaScript due to a bug in it.  But were it not
                    // for this bug the following could be used to
                    // load x3dom.js on the fly.

                    assert("The page did not include x3dom.js");

                    var head= document.getElementsByTagName('head')[0];
                    let script = document.createElement('SCRIPT');
                    script.onload = function() {
                        // proceed to hook up this newly loaded X3D node.
                        OnLoad();
                    };
                    head.appendChild(script);
                    script.src = "../x3dom/x3dom.js";
                } else {
                    // The X3DOM javaScript has been loaded already.
                    spew('\n\ncalling x3dom.reload()\n\n');

                    x3dom.runtime.ready = function() {
                        OnLoad();
                    };

                    x3dom.reload();
                }
            } else {
                dspew('GET ' + url + ' returned status ' + this.status);
            }
        };
        xhttp.send();
    }
}

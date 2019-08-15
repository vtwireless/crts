// Note: In order to keep webDesktop and scope usable by other software
// projects we cannot use require() in them so we must put a longer list
// of dependences in this CRTS specific code.
//
// require() is from /load.js

require('/session.js');
require('/spectrum/launcher.css');
require('/floatWidget/floatWidget.js');
require('/floatWidget/floatWidget.css');
require('/webtop/webtop.js');



/** Public interfaces in this file:
 *
 *
 *   function createSpectrumFeeds(io)
 *
 *      sets up spectrum feeds in the web socket io thingy
 *
 *      The feeds are not sent until a 'spectrumSubscribe'
 *      io command is sent.
 *
 *      @return a singleton object with the following method(s):
 *
 *
 *      addDisplayClass(name, jspath_or_func)
 *
 *         Adds a display of a given class type.
 *
 *         @param name is the name of the display type
 *
 *         @param jspath_or_func is the path to some javaScript with the
 *                 function named name or the function that constructs a
 *                 display
 *
 *                new "name"() must return an object with a
 *                   method draw() and optionally destroy().
 *
 *
 *     See file scopeDisplay.js function ScopeDisplay(desc, onclose = null)
 *     is an example spectrum display function.
 *
 */
function createSpectrumFeeds(io, opts = {}) {


    // We make the returned object be a singleton.
    //
    if(createSpectrumFeeds.count > 0) return createSpectrumFeeds.obj;
    ++createSpectrumFeeds.count;


    const defaultOpts = { parentNode: 'WTApp' };
    // Fill in missing options in argument opts with defaults.
    Object.keys(defaultOpts).forEach(function(key) {
        if(opts[key] === undefined)
            opts[key] = defaultOpts[key];
    });

    // spectrumId is spectrum feed ID that is currently selected.
    // spectrumId, when it is valid, it is the key to the current
    // spectrums[] object.
    //
    var spectrumId = -1;


    // This is the list of display classes that the user may use to show
    // a particular spectrum feed.  The created display objects must have
    // a draw() and a destroy().  The 'console' spectrum display object is
    // for debugging.
    //
    var displayClasses = { };

    // obj is the returned object.
    //
    var obj = { };

    var div = document.createElement('div');
    div.className = 'SPlauncher';

    let h2 =  document.createElement('h2');
    h2.appendChild(document.createTextNode('Spectrum Displays'));
    div.appendChild(h2);

    if(opts.parentNode === 'WTApp')
        new WTApp('Spectrum Feeds', div, {
            close: false /*do not let it close*/});
    else if(opts.parentNode)
        opts.parentNode.appendChild(div);
    else {
        document.body.appendChild(div);
        require('/js/showHide.js', function() {
            makeShowHide(div, { header: h2 });
        });
    }

    let container = document.createElement('div');
    div.appendChild(container);

    container.className = "selectContainer";
    let h3 = document.createElement('h3');
    container.appendChild(h3);
    h3.appendChild(document.createTextNode("USRP Feed Node"));
    var feedSelect = document.createElement('select');
    container.appendChild(feedSelect);
    feedSelect.className = 'SPlauncher'


    function putInLi(node) {
        let li = document.createElement('li');
        li.appendChild(node);
        return li;
    }


    function widgetChange(x, opts) {

        // TODO: Should the rate at this sends requests be locally
        // tallied and than sent at a lower rate.

        // Note: Video draw frame rates are about 60 Hz, and auto keyboard
        // repeat rates are about 30 Hz.

        if(spectrumId < 0) return;

        console.log(opts.label + ': ' + x + ' ' + opts.units);

        io.Emit("setSpectrumParameters", spectrumId,
            cFreqWidget.getValue(), bandwidthWidget.getValue(),
            updatePeriodWidget.getValue(), binsWidget.getValue());

    }

    var cFreqWidget, bandwidthWidget, binsWidget, updatePeriodWidget;


    let ul = document.createElement('div');
    container.appendChild(ul);
    ul.appendChild(putInLi(cFreqWidget =
        createFloatWidget(780.0, 990.0/*max*/, 700.0/*min*/, 2, {
            units: 'MHz', label: 'Freq', onchange: widgetChange})));
    ul.appendChild(putInLi(bandwidthWidget =
        createFloatWidget(1.0, 1000.0, 0.01, 2, {
            units: 'MHz', label: 'Bandwidth', onchange: widgetChange})));
    ul.appendChild(putInLi(binsWidget =
        createFloatWidget(10.0, 100.0, 2.0, 0, {
            label: 'Bins', onchange: widgetChange})));
    ul.appendChild(putInLi(updatePeriodWidget =
        createFloatWidget(0.1, 10.0, 0.01, 3, {
            units: 'Seconds', label: 'Update Period', onchange: widgetChange})));


    container = document.createElement('div');
    div.appendChild(container);
    container.className = "selectContainer";
    h3 = document.createElement('h3');
    container.appendChild(h3);
    h3.appendChild(document.createTextNode("Display Type"));
    var displaySelect = document.createElement('select');
    displaySelect.className = 'SPlauncher'
    container.appendChild(displaySelect);


    // To add a spectrum display of a certain type like 2D scope, 3D water
    // fall, debug spew, 2D waterfall or etc.
    //
    obj.addDisplayClass = function(name, jspath_or_func) {

        assert(typeof(displayClasses[name]) === 'undefined',
            'spectrum Display class "' +
            name + '" was already loaded.');

        // Add a display select option with this name.
        //
        var opt = document.createElement('option');
        opt.value = name;
        opt.appendChild(document.createTextNode(name));
        displaySelect.appendChild(opt);

        if(typeof(jspath_or_func) === 'function') {
            displayClasses[name] = jspath_or_func;
        } else {

            // jspath_or_func is a javaScript path to load via require().

            displayClasses[name] = function(desc, onclose) {

                // Here is the dummy display that does nothing until it gets
                // replaced by the javaScript that is loaded in require() just
                // below.
                var display = this;
                this.draw = function() {};
                this.destroy = function() { display = null; };

                require(jspath_or_func, function() {

                    assert(typeof(window[name]) === 'function', 'Path ' +
                        jspath_or_func + ' did not provide function named: ' +
                        name);

                    // Now we replace the dummy constructor function with a
                    // real display constructor function.  From here and
                    // after, when new displayClasses[name] is called, we will
                    // make a real display.
                    if(displayClasses[name] !== window[name])
                        displayClasses[name] = window[name];

                    if(display) {
                        // This is a dummy display, but now the real display
                        // javaScript is loaded and so we replace the dummy.
                        let o = new displayClasses[name](desc, onclose);
                        display.draw = o.draw;
                        display.destroy = o.destroy;
                    }
                });
            };
        }
    };


    if(true) // For debugging:
    obj.addDisplayClass('console debug', function(desc, onclose) {

        // Create a console debug display:
        //
        console.log('Made console debug spectrum display for ' + desc);
        this.draw = function(x, y) {
            console.log(desc + " DRAW: " + y);
        };
        this.destroy = function() {
            console.log('Removing "console debug" spectrum display for ' +
                desc);
            if(onclose) onclose();
        };
    });


    obj.addDisplayClass('2D Scope', '/spectrum/scopeDisplay.js');



    // A list of all the spectrum feeds that are currently available.
    var spectrums = {};

    io.On("spectrumUpdate", function(id, cFreq, bandwidth,
            updatePeriod, data) {
       /*
        console.log('got spectrumFeed(' + id + '): ' +
                    ' cFreq=' + cFreq + ' bandwidth=' + bandwidth +
                    ' bins=' + bins + ' period=' +  updatePeriod +
                    ' amplitudes=' + data + "\n\n");
                    */

        if(spectrums[id] === undefined ||
                Object.keys(spectrums[id].displays).length === 0) {
            io.Emit('spectrumUnsubscribe', id);
            return;
        }

        let spectrum = spectrums[id];
        let bins = data.length;

        // We do not need to generate a new x array unless parameters have
        // changed.
        if(spectrum.cFreq !== cFreq ||
                spectrum.bandwidth !== bandwidth ||
                spectrum.x.length !== bins) {
            let x = [];
            let deltaFreq = bandwidth/(bins-1);
            let startX = cFreq - bandwidth/(bins-1);

            if(bins === 1) {
                startX = cFreq;
                deltaFreq = 0.0;
            }

            for(let i=0 ;i<bins; ++i) {
                x[i] = startX + i * deltaFreq;
            }
            spectrum.x = x;
            spectrum.cFreq = cFreq;
            spectrum.bandwidth = bandwidth;
            spectrum.bins = bins;
        }

        Object.keys(spectrum.displays).forEach(function(key) {
            spectrum.displays[key].draw(spectrum.x,
                data, cFreq, bandwidth, updatePeriod);
        });
    });

    io.On('spectrumEnd', function(id) {
        console.log('Got "spectrumEnd" id=' + id);
        if(spectrums[id] !== undefined)
            spectrums[id].destroy();
    });


    function setFloatWidgets(spectrum) {

        cFreqWidget.setValue(spectrum.cFreq/1.0e6);
        bandwidthWidget.setValue(spectrum.bandwidth/1.0e6);
        binsWidget.setValue(spectrum.bins);
        updatePeriodWidget.setValue(spectrum.updatePeriod);
    }

    feedSelect.onchange = function() {

        if(feedSelect.length === 0) {
            spectrumId = -1;
            return;
        }

        let option = feedSelect.options[feedSelect.selectedIndex];
        spectrumId = option.value;

        assert(spectrums[spectrumId] !== undefined);

        console.log('Setting float widgets to spectrum feed id=' +
            spectrumId);
        setFloatWidgets(spectrums[spectrumId]);
    }


    io.On('newSpectrum', function(id, description, cFreq,
                bandwidth, updatePeriod, bins)  {

        console.log('Got "newSpectrum" [' + id + ']: "' +
                description + '"');
        assert(spectrums[id] === undefined, "We have spectrum id="
                + id + " already");

        var feedOption = document.createElement('option');
        feedOption.appendChild(document.createTextNode(description));
        feedOption.value = id;
        feedSelect.appendChild(feedOption);

        spectrums[id] = {
            destroy: function() {
                console.log('Removing spectrum option: ' + feedOption.value);
                feedOption.parentNode.removeChild(feedOption);
                Object.keys(spectrums[id].displays).forEach(function(key) {
                    if(spectrums[id].displays[key].destroy !== undefined)
                        spectrums[id].displays[key].destroy();
                });
                delete spectrums[id];
                feedSelect.selectedIndex = 0;
                feedSelect.onchange();
            },
            feedOption: feedOption,
            displays: {}, // list of current displays
            x: [],// the last array of X values
            cFreq: cFreq,
            bandwidth: bandwidth,
            updatePeriod: updatePeriod,
            bins: bins
        };

        if(spectrumId === -1) {
            feedSelect.selectedIndex = 0;
            spectrumId = feedSelect.options[feedSelect.selectedIndex].value;
            setFloatWidgets(spectrums[spectrumId]);
        }

        //io.Emit('spectrumSubscribe', id);
    });

    var displayCount = 0;



    var removeContainer = document.createElement('ul');
    removeContainer.className = 'removeContainer';
    div.appendChild(removeContainer);
    let button = document.createElement('button');
    button.appendChild(document.createTextNode("Create Plot"));
    button.className = 'SPlauncher';
    button.onclick = function() {

        let fi = feedSelect.selectedIndex;
        let di = displaySelect.selectedIndex;

        if(fi >= 0 && fi < feedSelect.length)
            console.log('feedSelect feed name=' +
                feedSelect.options[fi].value + ' ' +
                feedSelect.options[fi].firstChild.data);

        if(di >= 0 && di < displaySelect.length)
            console.log('displaySelect is: ' +
                displaySelect.options[di].value);

        if(!(fi >= 0 && fi < feedSelect.length &&
            di >= 0 && di < displaySelect.length))
            return;

        let id = feedSelect.options[fi].value;
        let name = displaySelect.options[di].value;
        let desc = id.toString() + ' ' +
            feedSelect.options[fi].firstChild.data;

        assert(spectrums[id] !== undefined,
            'There is no spectrum with id=' + id);
        assert(typeof(displayClasses[name]) === 'function',
            'There is no "' + name + '" display class');

        var displayId = displayCount++;

        var a = document.createElement('a');
        a.appendChild(document.createTextNode("Remove " +
            name + ' ' +
            displayId + ': ' + desc));
        a.href = "#";
        a.onclick = function() {

            if(spectrums[id] !== undefined &&
                    spectrums[id].displays[displayId] !== undefined) {
                spectrums[id].displays[displayId].destroy();
            }
        };
        removeContainer.appendChild(a = putInLi(a));


        var displayId = displayCount++;

        spectrums[id].displays[displayId] =
            new displayClasses[name](desc, function() {

            // onclose()
            if(spectrums[id] !== undefined &&
                    spectrums[id].displays[displayId] !== undefined) {
                delete spectrums[id].displays[displayId];
            }
            if(spectrums[id] !== undefined &&
                    Object.keys(spectrums[id].displays).length === 0) {
                io.Emit('spectrumUnsubscribe', id);
            }
            a.parentNode.removeChild(a);
        });
        if(Object.keys(spectrums[id].displays).length === 1)
            io.Emit('spectrumSubscribe', id);

    };
    removeContainer.appendChild(putInLi(button));

    return obj;
}

createSpectrumFeeds.count = 0;
createSpectrumFeeds.obj = null;

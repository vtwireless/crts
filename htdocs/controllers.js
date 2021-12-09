require('/session.js');


// It turns out that the javaScript getter and setter notation is not very
// useful for the parameter controller objects; as it forces the parameter
// object (example: freq) to only be used in setting and getting the value
// of the parameter and it can't be used for anything else, like setting a
// user callback for when the value changes.  We can set a "value"
// property of the parameter object like:
//
//    freq = getParameter(...)
//
//    freq.value = 56.4; // using set property
//
//    var val = freq.value; // using get property
//
// that does not seem like a big gain.


// The user may call Controllers(io) as many times as they like.
//
// Arguments:
//
// io   the socketIO like object that is built on a WebSocket.
//
// return an object that will have setters and getters in it
// as programs create them.
//
// TODO: Call this Parameters().
//
function Controllers(io) {

    var ret;

    // We make this function a singleton object.
    if(Controllers._ret)
        return Controllers._ret;

    var ret = Controllers._ret = {};

    // This is the object that will have all the parameter getters and
    // setters in it, and the internal state needed to make this interface
    // effective.
    var obj = {};

    // Helper function to make part of obj.  It should be private.
    //
    // Return true if this was ever called from
    // io.On('addController', ...)
    // for that parameter.
    //
    function initParameter(program, filter, parameter,
            addControllerCalled = true) {

        //console.log("initParameter(" + JSON.stringify(arguments) + ")");

        if(obj[program] === undefined)
            obj[program] = {};

        if(obj[program][filter] === undefined)
            obj[program][filter] = {};

        let par = obj[program][filter][parameter];

        if(par === undefined) {
            obj[program][filter][parameter] = {};
            par = obj[program][filter][parameter];
            par.getCallbacks = [];
            par.addControllerCalled = false;
            par.initParameterCallbacks = [];
            // The last set value before the is a parameter on the server.
            par.setValue = null;

            // This initial getter parameter value with get overwritten
            // very soon.
            //
            // TODO: the user needs to wait to use this value after the
            // first  io.On('getParameter',...) for this particular
            // parameter.
            //
            par.getValue = 0.0;
            // The user interface that is returned from
            // controller.parameter(...)
            par.api = {
                set: function(value) {
                    // Store value for now given we may not be able to
                    // send it to the server yet.
                    par.setValue = value;
                },
                get: function() {
                    return par.getValue;
                },
                addOnChange(callback) {
                    par.getCallbacks.push(callback);
                }
            };
        }

        if(addControllerCalled)
            // This parameter has been setup on the server.
            par.addControllerCalled = addControllerCalled;

        return par.addControllerCalled;
    }


    function postInitParameter(program, filter, parameter) {

        let par = obj[program][filter][parameter];

        // The parameter is setup on the server, so now we make the user
        // interface do more.
        if(par.setValue !== null) {
            // We write only the last value set.
            io.Emit('setParameter', program, filter, parameter, par.setValue);
            par.setValue = null;
        }

        while(cb = par.initParameterCallbacks.pop())
            cb(par.api);
    }


    io.On('addController', function(program, set, get, image) {


        Object.keys(set).forEach(function(filter) {

            Object.keys(set[filter]).forEach(function(parameter) {
                // If there is a setter than there will be a getter with
                // the same name.  If not, it will not matter, the server
                // will just ignore the 'subscribeToGetter' command.
                //
                // TODO: make this 'subscribeToGetter' optional somehow.
                //
                io.Emit('subscribeToGetter', program, filter, parameter);

                // We need an object with a key that does not collide with
                // an existing key.  We just assume that 'controls_xx'
                // does not collide with an existing key.  This
                // 'controls_xx' is not a user interface.
                //
                initParameter(program, filter, parameter);

                // The internal data for this parameter object, par.
                var par = obj[program][filter][parameter];
                var min = set[filter][parameter].min;
                var max = set[filter][parameter].max;

                par.api.set = function(val) {
                    // We'll clip the value between min and max.
                    if(val > max)
                        val = max;
                    else if(val < min)
                        val = min;
                    io.Emit('setParameter', program,
                            filter, parameter, val);
                };

                if(par.setValue !== null) {
                    par.api.set(par.setValue);
                    par.setValue = null;
                }

                // If this is rerunning the program that makes this
                // parameter we may be making this stuff again.
                delete par.api.min;

                Object.defineProperty(par.api, "min", {
                    configurable: true,
                    get : function (value) {
                        return min;
                    }
                });

                delete par.api.max;

                Object.defineProperty(par.api, "max", {
                    configurable: true,
                    get : function (value) {
                        return max;
                    }
                });


                postInitParameter(program, filter, parameter)
            });


            Object.keys(get[filter]).forEach(function(parameter) {

                // Some getters do not have a corresponding setter, so we
                // need to set them up here:
                if(set[filter] !== undefined &&
                        set[filter][parameter] !== undefined)
                    // This got setup above with both a setter and a
                    // getter.
                    return;

                io.Emit('subscribeToGetter', program, filter, parameter);

                initParameter(program, filter, parameter);

                postInitParameter(program, filter, parameter)
            });
        });

        console.log("io.On('addController', (" +
                JSON.stringify(arguments) + ")");

    });


    io.On('getParameter', function(program, filter, parameter, value) {

        let par;

        try {
            par = obj[program][filter][parameter];
        }  catch(ee) {
            initParameter(program, filter, parameter, false);
            par = obj[program][filter][parameter];
        }

        // Get a value for this parameter.  If the name spaces are not
        // consistent with what has been sent in 'addController' then we
        // handle it with try/catch.
        //
        try {
            // Next time javascript polls the value via the getter they
            // will get this value.
            par.getValue = value;

            // Event driven user stuff, from this getter change.
            par.getCallbacks.forEach(function(callback) {
                //console.log("calling callback("+value+")=" + callback);
                callback(value);
            });

            // The next time the user gets a value they will get this
            // value.  Example user getting a value to 'foo':
            //
            //    var foo = obj["program"]["filter"]["parameter"];
            //
        } catch(ee) {
            console.log("Got invalid 'getParameter' (" +
                [].slice.call(arguments) +
                ")");
            return;
        }
        //console.log("Got 'getParameter' (" +
        //        [].slice.call(arguments) + ")");
    });


    ret.parameter = function(startCallback, program, filter, parameter) {

        initParameter(program, filter, parameter, false);

        let par = obj[program][filter][parameter];

        if(par.addControllerCalled)
            startCallback(par.api);
        else
            par.initParameterCallbacks.push(startCallback);
    };


    return ret;
}


// We make this function return a singleton object.
Controllers._ret = null;


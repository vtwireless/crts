
selectFunctions = {

    // We need to convert the body of these functions to strings,
    // hence the odd text layout in these functions, they are
    // pushed to the left side.
    //
    // They are called like so:
    //
    // function callback(freq1, bw1, gn1, mcs1, bytes1, freq2, bw2, gn2, mcs2, bytes2, dt, userData, init)
    //

    "Largest contiguous interference-free sub-band":
function() {
    // Make Signal 1 occupy the largest contiguous interference-free sub-band
    // freq1 and freq2 are in Hz
    // freq2 and bw2 are assumed to be known

    var mid_freq = min_freq + (max_freq - min_freq)/2.0;

    if(freq2 > mid_freq) {
        freq1 = (freq2 - bw2/2.0 + min_freq)/2.0;
        bw1 = 2.0*(freq1 - min_freq);
    } else {
        freq1 = (freq2 + bw2/2.0 + max_freq)/2.0;
        bw1 = 2.0*(max_freq - freq1);
    };

    // We have limited bandwidth.
    if(bw1 > max_bw)
        bw1 = max_bw;
    else if(bw1 < min_bw)
        bw1 = min_bw;

   return { freq1: freq1, bw1: bw1 };
},



    "Changing freq1":
function() {
    // freq1 is in Hz
    freq1 += 0.04e6;
    if(freq1 > max_freq1)
        freq1 = min_freq1;

    return { freq1: freq1 };
},


    "Spew":
function() {
    if(typeof userData.count === 'undefined')
        userData.count = 0;
    console.log("HELLO " + userData.count + " : " + [].slice.call(arguments));
    ++userData.count;
},



    "Changing freq2":
function() {
    freq2 += 0.04e6;
    if(freq2 > max_freq2)
        freq2 = min_freq2;

    return { freq2: freq2 };
},




    "Changing bw1":
function() {
    bw1 += 0.02e6;
    if(bw1 > max_bw1)
        bw1 = min_bw1;

    return { bw1: bw1 };
},


    "Changing bw2":
function() {
    bw2 += 0.02e6;
    if(bw2 > max_bw2)
        bw2 = max_bw2;

    return { bw2: bw2 };
},



    "Changing gn1":
function() {
    gn1 += 0.5;
    if(gn1 > max_gn1)
        gn1 = max_gn1;

    return { gn1: gn1 };
},



    "Changing gn2":
function() {
    // Note this gain change is to low to have an effect.
    // libUHD will not change the gain this small amount.
    gn2 += 0.2;
    if(gn2 > max_gn2)
        gn2 = max_gn2;

    return { gn2: gn2 };
},



    "Changing mcs1":
function() {

    if(typeof userData.t === undefined)
        userData.t = 0.0;

    userData.t += dt;

    if(userData.t < 3.0) return;

    // Reset timer, keeping remainder.
    userData.t -= 3.0;

    mcs1 += 1;
    if(mcs1 > max_mcs1)
        mcs1 = max_mcs1;

    return { mcs1: mcs1 };
},


    "Changing mcs1 with bytes out":
function() {

    function reset() {
        userData.t = 0.0;
        userData.done = false;
        userData.lastBytes = bytes1;
    }

    if(init)
        reset();

    if(userData.done) return;

    userData.t += dt; // in seconds

    const measureT = 5.0;

    if(userData.t < measureT) return;

    // Total bytes over 5 seconds.
    var dBytes = bytes1 - userData.lastBytes;

    console.log("   dBytes=" + dBytes);

    reset();

    if(dBytes < 1000) {
        // Bummer, not thinking any more.
        userData.done = true;
        return { mcs1: mcs1-1 };
    }

    if(mcs1 < max_mcs1) {
        // so try for more throughput.
        return { mcs1: mcs1+1 };
    }

    // or not.

},


    "freq1 -- Random walk frequency example":
function() {

    // Random walk frequency example
 
    // Initialize / update userData.freq1, used to set freq1 (freq1 is in Hz)
    if(init)
        userData.freq1 = 919.5e6;
    else
        userData.freq1 += -2.0e5 + 4e5*Math.random();
 
    // userData is automatically saved.
    return { freq1: userData.freq1 };
},


    "Blank":
function() {

    // Paste your code in here.

}



};


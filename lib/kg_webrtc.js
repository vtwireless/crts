//Datachannel globals
var connectButton = document.getElementById('connectButton');
var disconnectButton = document.getElementById('disconnectButton');
var sendButton = document.getElementById('sendButton');
var messageInputBox = document.getElementById('message');
var receiveBox = document.getElementById('receivebox');

 // Set event listeners for user interface widgets

//connectButton.addEventListener('click', connectPeers, false);
//disconnectButton.addEventListener('click', disconnectPeers, false);
sendButton.addEventListener('click', sendMessage, false);

var localConnection; //RTCPeerConnection for local connection
var remoteConnection; //RTCPeerConnection for remote connection

var sendChannel; //RTCDataChannel for local(sender)
var receiveChannel; //RTCDataChannel for remote (receiver)

// Create a random room if not already present in the URL.
var isInitiator;
var room = window.location.hash.substring(1);
if (!room) {
  room = window.location.hash = randomToken();
}

//ICE Candidates -- can be added to opt
//will try it uncommented first as in step6
/*var configuration = {
  'iceServers': [
    {'urls': 'stun:stun.stunprotocol.org:3478'},
    {'urls': 'stun:stun.l.google.com:19302'},
  ]
};
*/

/****************************************************************************
* Signaling server
****************************************************************************/
On('ipaddr', function(ipaddr) {
  spew('Server IP address is: ' + ipaddr);
  // updateRoomURL(ipaddr);
});

On('created', function(room, clientId) {
  spew('Created room', room, '- my client ID is', clientId);
  isInitiator = true;
//  grabWebCamVideo();
});

On('joined', function(room, clientId) {
  spew('This peer has joined room', room, 'with client ID', clientId);
  isInitiator = false;
  createPeerConnection(isInitiator, configuration);
//  grabWebCamVideo();
});

//creates new room if the current one is full. removes current hash, reloads window
On('full', function(room) {
  alert('Room ' + room + ' is full. We will create a new room for you.');
  window.location.hash = '';
  window.location.reload();
});

On('ready', function() {
  spew('Socket is ready');
  createPeerConnection(isInitiator, configuration);
});

On('log', function(array) {
  console.log.apply(console, array);	//will spew work as spew.apply?
});

On('message', function(message) {
  spew('Client received message:', message);
  signalingMessageCallback(message);
});

// Joining a room.
Emit('create or join', room);

if (location.hostname.match(/localhost|127\.0\.0/)) {
  Emit('ipaddr');
}

// Leaving rooms and disconnecting from peers.
On('disconnect', function(reason) {
  spew(`Disconnected: ${reason}.`);
  connectButton.disabled = false;
  disconnectButton.disabled = true;
  sendButton.disabled = true;
});

On('bye', function(room) {
  spew(`Peer leaving room ${room}.`);
  sendButton.disabled = true;
  // If peer did not create the room, re-enter to be creator.
  if (!isInitiator) {
    window.location.reload();
  }
});

window.addEventListener('unload', function() {
  spew(`Unloading window. Notifying peers in ${room}.`);
  Emit('bye', room);
});


/**
* Send message to signaling server
*/
function sendMessage(message) {
  spew('Client sending message: ', message);
  Emit('message', message);
}

/****************************************************************************
* WebRTC peer connection and data channel
****************************************************************************/

var peerConn;
var dataChannel;

function signalingMessageCallback(message) {
  if (message.type === 'offer') {
    spew('Got offer. Sending answer to peer.');
    peerConn.setRemoteDescription(new RTCSessionDescription(message), function() {}, logError);
    peerConn.createAnswer(onLocalSessionCreated, logError);
  }
  else if (message.type === 'answer') {
    spew('Got answer.');
    peerConn.setRemoteDescription(new RTCSessionDescription(message), function() {}, logError);
  } 
  else if (message.type === 'candidate') {
    peerConn.addIceCandidate(new RTCIceCandidate({
      candidate: message.candidate
    }));
  }
}

function createPeerConnection(isInitiator, config) {
  spew('Creating Peer connection as initiator?', isInitiator, 'config:', config);
  peerConn = new RTCPeerConnection(config);

// send any ice candidates to the other peer
peerConn.onicecandidate = function(event) {
  spew('icecandidate event:', event);
  if (event.candidate) {
    sendMessage({
      type: 'candidate',
      label: event.candidate.sdpMLineIndex,
      id: event.candidate.sdpMid,
      candidate: event.candidate.candidate
    });
  } else {
    spew('End of candidates.');
  }
};

if (isInitiator) {
  spew('Creating Data Channel');
  dataChannel = peerConn.createDataChannel('photos');
  onDataChannelCreated(dataChannel);

  spew('Creating an offer');
  peerConn.createOffer(onLocalSessionCreated, logError);
} else {
  peerConn.ondatachannel = function(event) {
    spew('ondatachannel:', event.channel);
    dataChannel = event.channel;
    onDataChannelCreated(dataChannel);
  };
}
}

function onLocalSessionCreated(desc) {
  spew('local session created:', desc);
  peerConn.setLocalDescription(desc, function() {
    console.log('sending local desc:', peerConn.localDescription);
    sendMessage(peerConn.localDescription);
  }, logError);
}

function onDataChannelCreated(channel) {
  spew('onDataChannelCreated:', channel);

  channel.onopen = function() {
    spew('CHANNEL opened!!!');
    connectButton.disabled = false;
    disconnectButton.disabled = true;
    sendButton.disabled = false;

  };

  channel.onclose = function () {
    console.log('Channel closed.');
    connectButton.disabled = true;
    disconnectButton.disabled = false;
    sendButton.disabled = false;
  }

//checks whether browser is firefox or chrome because webrtc behaves differently for each?
/*  channel.onmessage = (adapter.browserDetails.browser === 'firefox') ?
  receiveDataFirefoxFactory() : receiveDataChromeFactory();
*/
}

function sendMessage() {
// Split data channel message in chunks of this byte length.
	var cnt= 0;
	spew('sending cnt: ' + cnt);
	if (!dataChannel) {
  		logError('Connection has not been initiated. ' +
    	'Get two peers in the same room first');
  	return;
	} 
	else if (dataChannel.readyState === 'closed') {
  		logError('Connection was lost. Peer closed the connection.');
  	return;
	}
	var i;
	for (i = 0; i < 100; i++) {
		dataChannel.send(cnt);
		cnt++;	
	}	
}

function randomToken() {
  return Math.floor((1 + Math.random()) * 1e16).toString(16).substring(1);
}

function logError(err) {
  if (!err) return;
  if (typeof err === 'string') {
    console.warn(err);
  } else {
    console.warn(err.toString(), err);
  }
}


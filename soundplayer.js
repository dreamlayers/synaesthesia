/* Synaesthesia - program to display sound graphically
   Copyright (C) 1997  Paul Francis Harrison

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  675 Mass Ave, Cambridge, MA 02139, USA.

  The author may be contacted at:
    pfh@yoyo.cc.monash.edu.au
  or
    27 Bond St., Mt. Waverley, 3149, Melbourne, Australia
*/

/* Started with http://jsfiddle.net/gaJyT/18/
 * Changed to play from audio element using object URL
 * instead of decompressing whole track into memory.
 */

var context;
var audio;
var source;
var processor;
var displaySynaesthesia;
var dataBuffer;
const downFactor = 2;
const NumSamples = 1024;

function initSyn() {
    displaySynaesthesia = Module.cwrap('displaySynaesthesia',
                                       'number', ['number']);
    dataBuffer = new Float32Array(Module.HEAPF32.buffer,
                                  Module.ccall('getDataBuffer', 'number', []),
                                  NumSamples * 2);
}

function initAudio(data) {
    if (!context) {
      context = new (window.AudioContext || window.webkitAudioContext)();
    }
    if (!audio) {
      initSyn();
      audio = new Audio();
    }
    audio.src = data;
    if (!source) {
      source = context.createMediaElementSource(audio);
      connectAudio();
    }
    audio.play();
}

function connectAudio() {
    // Chrome only runs the processor if it has outputs and is
    // connected to the destination.
    processor = context.createScriptProcessor(NumSamples * downFactor,
                                              2, 2);
    processor.onaudioprocess = processAudio;
    source.connect(processor);
    processor.connect(context.destination);
    source.connect(context.destination);
}

function disconnectAudio() {
    source.disconnect(0);
    processor.disconnect(0);
}

function processAudio(e) {
    var leftSamples = e.inputBuffer.getChannelData(0);
    var rightSamples = e.inputBuffer.getChannelData(1);

    for (var i = 0; i < NumSamples*2; i += 2) {
        dataBuffer[i] = (leftSamples[i] + leftSamples[i+1]) * 16384;
        dataBuffer[i+1] = (rightSamples[i] + rightSamples[i+1]) * 16384;
    }
    displaySynaesthesia();
}

function dropEvent(evt) {
    evt.stopPropagation();
    evt.preventDefault();

    var droppedFiles = evt.dataTransfer.files;
    var oldurl;
    if (audio) {
        oldurl = audio.src;
    }
    Module.print("Playing: " + droppedFiles[0].name);
    initAudio(window.URL.createObjectURL(droppedFiles[0]));
    if (oldurl) {
        window.URL.revokeObjectURL(oldurl)
    }
}

function dragOver(evt) {
    evt.stopPropagation();
    evt.preventDefault();
    return false;
}

var dropArea = document.body;
dropArea.addEventListener('drop', dropEvent, false);
dropArea.addEventListener('dragover', dragOver, false);
Module.print("Drag an audio file and drop it here to start.");
Module.print("Use any format supported by your browser.");
Module.print("Most browsers support MP3 files.");

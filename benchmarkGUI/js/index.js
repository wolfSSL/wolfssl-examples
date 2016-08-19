'use strict';

var spawn = require('child_process').spawn;
var StringDecoder = require('string_decoder').StringDecoder;
var gauges = {};
var benchmarkProcess;
var benchmarkArgs = [];
var processName = 'benchmark';

// Options for gauges
var options = {
  'AES' : {
    divID           : "aesGauge",
    gaugeUnits      : "MB/s",
    maxVal          : 20,
    tickSpaceMajVal : 5,
    tickSpaceMinVal : 1
  },
  'SHA' : {
    divID           : "shaGauge",
    gaugeUnits      : "MB/s",
    maxVal          : 200,
    tickSpaceMajVal : 25,
    tickSpaceMinVal : 5
  },
  'RSA' : {
    divID           :   "rsaGauge",
    gaugeUnits      :   "milliseconds",
    maxVal          :   10,
    tickSpaceMajVal :   2,
    tickSpaceMinVal :   .4
  }
}

/* Typical jquery run when the dom is ready */
$(function() {

  var win = nw.Window.get();

  /* Zoom slider Setup */
  $('#sizeSlider').slider({
    min: 0.5,
    max: 3,
    step: 0.05,
    tooltip_position:'bottom'
  });

  /* Register Events */
  win.on('close', function (){
    /* Close benchmark process on program exit */
    console.log('Closing window...');
    this.hide();
    closeProcess();
    this.close(true);
  });

  $('#demoToggle').change(function() {
    /* Event for starting demo based on the demo toggle */
    console.log('Starting demo...');
    if($(this).prop('checked')) {
      startDemo();
      // Go into full screen mode.
      win.toggleKioskMode();
      // Set default zoom
      $("#sizeSlider").slider('setValue', 1.5);
      $('.gaugeContainer').css('zoom', $('#sizeSlider').slider('getValue'));
    } else {
      $('#aesToggle').bootstrapToggle('enable');
      $('#shaToggle').bootstrapToggle('enable');
      $('#rsaToggle').bootstrapToggle('enable');
      win.toggleKioskMode();

      closeProcess();
      removeAllGauges();
      $("#sizeSlider").slider('setValue', 1);
    }
  });

  $("#sizeSlider").on("slide", function(slideEvt) {
    /* Event for the zoom slider to update CSS */
    $('.gaugeContainer').css('zoom', slideEvt.value)
  });

  /* TODO: Refactor into function. */
  $('#aesToggle').change(function() {
    if($(this).prop('checked')) {
      console.log('Starting AES benchmark...');
      benchmarkArgs.push('-ag');
      gauges['AES'] = new gauge('AES', 'AES-GCM 192', options['AES']);
      $('.gaugeContainer').css('zoom', $('#sizeSlider').slider('getValue'));
      startProcess();
    } else {
      removeArg('-ag');
      removeGauge('AES');
      startProcess();
    }
  });

  $('#shaToggle').change(function() {
    if($(this).prop('checked')) {
      console.log('Starting SHA benchmark...');
      benchmarkArgs.push('-s256');
      gauges['SHA'] = new gauge('SHA', 'SHA 256', options['SHA']);
      $('.gaugeContainer').css('zoom', $('#sizeSlider').slider('getValue'));
      startProcess();
    } else {
      removeArg('-s256');
      removeGauge('SHA');
      startProcess();
    }
  });

  $('#rsaToggle').change(function() {
    if($(this).prop('checked')) {
      console.log('Starting RSA benchmark...');
      benchmarkArgs.push('-rd');
      gauges['RSA'] = new gauge('RSA', 'RSA 2048 Dec', options['RSA']);
      $('.gaugeContainer').css('zoom', $('#sizeSlider').slider('getValue'));
      startProcess();
    } else {
      removeArg('-rd');
      removeGauge('RSA');
      startProcess();
    }
  });

  $('#sgxToggle').change(function(){
    if($(this).prop('checked')) {
      console.log('Displaying sgx header.');
      /* Add SGX header.  Comment out if not wanted */
      $('#programTitle').html('wolfSSL + Intel<sup>&reg;</sup> SGX');
      console.log('Switching to SGX benchmark');
      processName = 'benchmark-SGX';
      if(benchmarkProcess) {
        startProcess();
      }
    } else {
      $('#programTitle').html('wolfSSL');
      console.log('Switching to regular benchmark');
      processName = 'benchmark';
      if(benchmarkProcess) {
        startProcess();
      }
    }
  });
  // Show window when ready
  win.show();
});

/**
* Start demo.
*/
function startDemo(){

  /* Disable other toggles */
  $('#aesToggle').bootstrapToggle('off').bootstrapToggle('disable');
  $('#shaToggle').bootstrapToggle('off').bootstrapToggle('disable');
  $('#rsaToggle').bootstrapToggle('off').bootstrapToggle('disable');

  /* Create gauges */
  gauges['AES'] = new gauge('AES', 'AES-GCM 192', options['AES']);
  gauges['SHA'] = new gauge('SHA', 'SHA 256', options['SHA']);
  gauges['RSA'] = new gauge('RSA', 'RSA 2048 Dec', options['RSA']);

  benchmarkArgs.push('-ag');
  benchmarkArgs.push('-s256');
  benchmarkArgs.push('-rd');

  startProcess();

  console.log("Demo started.");
}

/**
* Start the benchmark process with given arguments.
*/
function startProcess() {
  // First, close any possibly running process.
  closeProcess();

  if(benchmarkArgs.length > 0) {
    // Start process with configured arguments
    benchmarkProcess = spawn('./' + processName, benchmarkArgs);
    console.log('Process started.');

    // Register events for child process
    var decoder = new StringDecoder('utf8');
    benchmarkProcess.stdout.on('data', data => {
        var chunk = decoder.write(data);
        var parsedChunk = +parseFloat(chunk).toFixed(2);
        console.log(chunk);

        // Figure out which gauge to update.
        // Go through keys of gauges and check if they exist in the event data
        for(var key in gauges) {
          if(chunk.includes(key)) {
            gauges[key].updateData(parsedChunk);
          }
        }
    });

    benchmarkProcess.stderr.on('data', (data) => {
      var chunk = decoder.write(data);
      console.log('err: ' + chunk);
    });

    benchmarkProcess.on('close', (code) => {
      console.log('process exited');
    });

    benchmarkProcess.on('error', (err) => {
      console.log('Process error: ' + err);
      closeProcess();
      // Display error on GUI
      $('#errorText').html(err);
      $('#guiError').removeClass('hidden');
    });
  }
}

/* Close process */
function closeProcess() {
  // Only exit if the process exists
  if(benchmarkProcess)
  {
    console.log('Closing chld process...');
    benchmarkProcess.kill('SIGTERM');
  }
  var errorDiv = $('#guiError');
  if(!errorDiv.hasClass('hidden')) {
    errorDiv.addClass('hidden');
  }
}

function removeAllGauges() {
  // Just deletes all gauges from GUI.
  console.log('Removing gauges...')
  for (var key in gauges) {
    console.log('Removing gauge: ' + key);
    removeGauge(key);
  }
}

function removeGauge(key) {
  // Deletes the specified gauge from the GUI.
  if(gauges.hasOwnProperty(key)) {
    gauges[key].removeGauge();
    delete gauges[key];
  }
}

function removeArg(argument){
  var argIndex = benchmarkArgs.indexOf(argument);
  if (argIndex > -1) {
    benchmarkArgs.splice(argIndex, 1);
  }
}

/**
* Acts as a wrapper around the d3gauge object.
*/

function gauge(newKey, title, options) {
  this.key = newKey;
  this.title = title;
  this.d3gauge;
  this.avg = 0;
  this.total = 0;
  this.count = 0;
  /* Default options for all gauges. */
  this.options = {
    gaugeRadius : 145,
    needleVal   : 0,
    minVal      : 0
  };

  $.extend(this.options, options);

  /* Adds necessary HTML for the gauge and draws the gauge. */
  this.create = () => {
    $('#gauges').append('<div class="pull-left gaugeContainer" id="' + this.key + 'GaugeContainer">' +
                          '<div><h4 class="text-center"><strong>' + this.title + '</strong></h4></div>' +
                          '<div id="' + this.key + 'Gauge" class="demoGauge"></div>' +
                          '<div class="row">' +
                            '<div  class="col-xs-3 col-xs-offset-1 text-center"><h4><strong>Min</strong></h4></div>' +
                            '<div  class="col-xs-4 text-center"><h4><strong>Avg</strong></h4></div>' +
                            '<div  class="col-xs-3 text-center"><h4><strong>Max</strong></h4></div>' +
                          '</div>' +
                          '<div class="row">' +
                            '<div  class="col-xs-3 col-xs-offset-1 text-center"><h4 id="' + this.key + 'Min"></h4></div>' +
                            '<div  class="col-xs-4 text-center"><h4 id="' + this.key + 'Avg"></h4></div>' +
                            '<div  class="col-xs-3 text-center"><h4 id="' + this.key + 'Max"></h4></div>' +
                          '</div>' +
                        '</div>');

    /* Draw the gauge from d3gauge */
    this.d3gauge = new drawGauge(this.options);
  };

  /**
  * Removes self.
  */
  this.removeGauge = () => {
      $('#' + this.key + 'GaugeContainer').remove();
  };

  this.updateData = (data) => {
    // Update the gauge's data.
    this.d3gauge.updateGauge(data);

    /* Update min/max/avg */
    this.total += data;
    this.count += 1;
    this.avg = +(this.total/this.count).toFixed(2);
    console.log(this.key + " average: " + this.avg);
    $('#' + this.key + 'Avg').html(this.avg);

    // Check current min, replace if necessary
    if(!this.min || data < this.min) {
      console.log(this.key + " min: " + this.min);
      this.min = data;
      $('#' + this.key + 'Min').html(this.min);
    };

    // Check current max, replace if necessary
    if(!this.max || data > this.max) {
      console.log(this.key + " max: " + this.max);
      this.max = data;
      $('#' + this.key + 'Max').html(this.max);
    }
  };

  /* Finally, display gauge */
  this.create();
};

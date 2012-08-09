/**
 * Scorecreep Javascript
 */
jQuery(document).ready(function($) {
  var $sel = $('#edit-field-performance-team-und'),
    $perfList = $('#edit-field-performance-performer-und'),
    $performersOrig = $('#edit-field-performance-performer-und option').clone();
  $('#edit-field-performance-performer-und option').remove();
  $sel.change(function (e) {
    var selected = $(this).find('option:selected').text(),
      current = [],
      $performers = $performersOrig.clone();
    $('#edit-field-performance-performer-und option').remove();
    $perfList.append($performers);
    $perfList.find('option').each(function (index, item) {
      if (index > 0 && $(item).text().indexOf(selected) === -1) {
        $(item).remove();
      }
    });
    $perfList.children('option').eq(0).attr('selected', 'selected');
  });

  if ($sel.val() !== '_none') {
    $sel.trigger('change');
  }
});

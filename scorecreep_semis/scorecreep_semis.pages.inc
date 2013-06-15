<?php
/**
 * @file scorecreep_semis.pages.php
 *
 * Scorecreep Semis Page handlers
 */

/**
 * Display the tournament_bouts_list view, passing in the tid for the tournament.
 */
function scorecreep_semis_bouts_list($node) {
  drupal_set_title($node->title . ' : Semi-final Bouts');
  $view = views_get_view('tournament_semifinals_list');
  $term = scorecreep_get_tournament_term($node);
  $args = array();
  if (isset($term)) {
    $args[] = $term->tid;
  }
  return $view->execute_display('default', $args);
}


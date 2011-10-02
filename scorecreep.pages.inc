<?php

function bout_page_performances ($node) {
  //$bout_tid = bout_term_id_load($node);
  //dvm($node);
  if ($node->type != 'scorecreep_bout') {
    return FALSE; //TODO FIX THIS
  }

  drupal_set_title($node->title . " : Performances by Rank");
  $uri = entity_uri('node', $node);
  // Set the node path as the canonical URL to prevent duplicate content.
  drupal_add_html_head_link(array('rel' => 'canonical', 'href' => url($uri['path'], $uri['options'])), TRUE);
  // Set the non-aliased path as a default shortlink.
  drupal_add_html_head_link(array('rel' => 'shortlink', 'href' => url($uri['path'], array_merge($uri['options'], array('alias' => TRUE)))), TRUE);
 // return node_show($node);
  $bout_id = $node->field_bout['und'][0]['tid'];

  $performance_ranks = _scorecreep_get_performances_by_rank($bout_id);
  $output = '';
  if (!empty($performance_ranks)) {
    // Make a table for them.
    $perf_arr = array();
    for ($i = 0; $i < count($performance_ranks); $i++) {
        $perf_arr[] = array(
          'rank' => $performance_ranks[$i]['rank'],
          'team_name' => $performance_ranks[$i]['team_name'],
          'performer' => $performance_ranks[$i]['performer'],
          'group' => $performance_ranks[$i]['group'],
          'rotation' => $performance_ranks[$i]['rotation'],
          'score' => $performance_ranks[$i]['score'],
        );
    }
    $header = array( t('Rank'),t('Team'), t('Performer(s)'), t('Group Piece'),t('Rotation'), t('Score'));
    $output .= theme('table', array('header' => $header, 'rows' => $perf_arr));
  }
  else {
    $output = t('No scores in this bout.');
  }
  return $output;

}

function scorecreep_tournament_page($term) {
  $tournament = array_shift(array_values(taxonomy_get_term_by_name($term)));
  $nps_bout_terms = taxonomy_get_children($tournament->tid);
  $bouts = array();
  drupal_set_title($tournament->description);
  /*
  foreach ($nps_bout_terms as $bout) {
    $name = check_plain($bout->name);
    $id = str_replace("Bout ", "", $name);
    $path = 'scorecreep/bout/' . check_plain($term) . '-' . $id;
    $bouts[] = l($name, $path);   
  }
  */

  $output = '';//t($tournament->description);
  $output = l('Prelim Scores', 'scorecreep/' . check_plain($term) . '/scores');
  /*
  if (!empty($bouts)) {
    $output .= theme_item_list(array('items' => $bouts, 'title' => 'Bouts', 'type' => 'ul', 'attributes' => array('id' => 'bout_list')));
  } else {
    $output .= t('No bouts set for this tournament.');
  }
  */
  return $output;
}

function tournament_scores_page($term) {
  $tournament = array_shift(array_values(taxonomy_get_term_by_name($term)));
  $nps_bout_terms = taxonomy_get_children($tournament->tid);
 
  drupal_set_title($tournament->description . ' : Prelim Scores');
  $totals = _scorecreep_get_ranks_for_all_teams($tournament->tid);  
  $output = t('');
  if (!empty($totals)) {
    foreach ($totals as $r) {
      $rows[] = array_map('check_plain', $r);
    }
    $header = array(t('Place'), t('Team'), t('Bout 1 Rank'), t('Bout 1 Score'), t('Bout 2 Rank'),  t('Bout 2 Score'), t('Total Rank'), t('Strength'), t(' Total Score'));
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
  }
  else {
    $output .= t('There are no bouts recorded for this tournament yet.');
  }
  return $output;
}

function tournament_scores_error($term) {
	return t("Oops! Having a little trouble! We are looking into it and will have this page fixed as soon as possible.");
}

function tournament_group_scores_page($term) {
  $tournament = array_shift(array_values(taxonomy_get_term_by_name($term)));
  $nps_bout_terms = taxonomy_get_children($tournament->tid);
 
  drupal_set_title($tournament->description . ' : Group Piece Scores');
  $totals = _scorecreep_get_ranks_for_all_group_pieces($tournament->tid); 
  $output = t('The below list includes teams in semi-finals. ');
  if (!empty($totals)) {
    foreach ($totals as $r) {
      $rows[] = array_map('check_plain', $r);
    }
    $header = array(t('Rank in Bout'), t('Team'), t('Bout'), t('Rotation'), t('Score'));
    $output .= theme('table', array('header' => $header, 'rows' => $rows));
  }
  else {
    $output .= t('There are no bouts recorded for this tournament yet.');
  }
  return $output;
}


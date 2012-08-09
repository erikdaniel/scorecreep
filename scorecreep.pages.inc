<?php
/**
 * Display the tournament_teams_list view, passing in the tid for the tournament.
 */
function tournament_teams_page($node) {
  drupal_set_title($node->title . ' : Teams');
  $view = views_get_view('tournament_teams_list');
  $term = scorecreep_get_tournament_term($node);
  $args = array();
  if (isset($term)) {
    $args[] = $term->tid;
  }
  return $view->execute_display('default', $args);
}


/**
 * Display the tournament_bouts_list view, passing in the tid for the tournament.
 */
function scorecreep_bouts_list($node) {
  drupal_set_title($node->title . ' : Bouts');
  $view = views_get_view('tournament_bouts_list');
  $term = scorecreep_get_tournament_term($node);
  $args = array();
  if (isset($term)) {
    $args[] = $term->tid;
  }
  return $view->execute_display('default', $args);
}



function bout_page_performances ($node) {
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

function tournament_scores_page($node) {
  // Overrides individual tournament
  if ($node->type == 'individual_tournament' && function_exists('iwps_scores_page')) {
    return iwps_scores_page($node);
  }

  drupal_set_title($node->title . ' : Prelim Scores');
  $totals = array();
  $tournament = scorecreep_get_tournament_term($node);
  if (isset($tournament)) {
    $nps_bout_terms = taxonomy_get_children($tournament->tid);
    $totals = _scorecreep_get_ranks_for_all_teams($tournament->tid);  
  }
  else {
    $nps_bout_terms = array();
  }
 
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

function tournament_group_scores_page($node) {
  $tournament = scorecreep_get_tournament_term($node);
  $nps_bout_terms = array();
  $totals = array();

  if (isset($tournament)) {
    $nps_bout_terms = taxonomy_get_children($tournament->tid);
    $totals = _scorecreep_get_ranks_for_all_group_pieces($tournament->tid); 
  }
 
  drupal_set_title($node->title . ' : Group Piece Scores');

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


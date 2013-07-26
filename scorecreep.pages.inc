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
  $content = array(
    'list' => array(
      '#markup' => $view->execute_display('default', $args),
    ),
  );

  if (node_access('update', $node)) {
    $content['add_team_form'] = drupal_get_form('scorecreep_add_team_form', $node);
  }
  return $content;
}

function scorecreep_add_team_form($form, &$form_state) {
    $form['#attributes']['class'][] = 'form-horizontal';
    $form['team'] = array(
        '#title' => t('Add Team'),
        '#type' => 'fieldset',
        '#description' => t('Add a team to this tournament'),
    );
    $form['team']['team_name'] = array(
        '#title' => t('Team name'),
        '#type' => 'textfield',
        '#required' => TRUE,
        '#maxlength' => 60,
        '#size' => 30,
    );
    $form['team']['city'] = array(
        '#title' => t('City'),
        '#type' => 'textfield',
        '#required' => TRUE,
        '#maxlength' => 50,
        '#size' => 30,
    );
    $form['team']['state_province'] = array(
        '#title' => t('State/Province'),
        '#type' => 'textfield',
        '#required' => TRUE,
        '#maxlength' => 5,
        '#size' => 5,
    );
    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit')
    );
    return $form;
}

/**
 * Handle team add validations.
 */
function scorecreep_add_team_form_validate($form, &$form_state) {

}


/**
* Handle post-validation form submission.
*/
function scorecreep_add_team_form_submit($form, &$form_state) {
    $name = $form_state['values']['team_name'];
    $city = $form_state['values']['city'];
    $state_province = $form_state['values']['state_province'];
    $tournament_node = array_pop($form_state['build_info']['args']);

    $term = scorecreep_get_tournament_term($tournament_node);

    $node = new stdClass();
    $node->type = strtolower(SCORECREEP_TEAM_NODE_TYPE);
    node_object_prepare($node);

    $node->title = check_plain($name);
    $node->language = LANGUAGE_NONE;
    $node->field_term_tournament[$node->language][0]['tid'] = $tournament_node->field_category[$tournament_node->language][0]['tid'];
    $node->field_team_city[$node->language][0]['value'] = check_plain($city);
    $node->field_team_state[$node->language][0]['value'] = check_plain($state_province);

    node_save($node);

    drupal_set_message(t('%name added to %tourney.',
        array('%name' => $name, '%tourney' => $tournament_node->title )));

}


/**
 * Tournament Administration page.
 */
function tournament_admin_page($node) {

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


/**
 * Display the performances list on the bout page.
 */
function bout_page_performances ($node) {
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
          'group' => ( $performance_ranks[$i]['group'] ) ? "<span class='icon-check'></span>": '', //TODO this should be in the theme
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


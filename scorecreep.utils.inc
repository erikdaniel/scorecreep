<?php
define("NUM_ROTATIONS", 4);

/** 
 * Callback to return the node id from a given string.
 *
 * @param $str
 *  A bout node id OR bout node alias.
 */
function bout_id_or_alias_load($str) {
  /* This may be a better way
  if (arg(0) == 'node') {
      $nid = arg(1);
  }
  */
  $bid = FALSE;
  if (is_int($str)) {
    $bid = $str;
  }
  else {
    $bid = drupal_lookup_path('source', 'scorecreep/bout/' . $str); 
    if ($bid) {
      $arr = explode('/', $bid);
      $bid = $arr[1];
    }
  }
  return $bid;
}



/**
 * Get total scores in about and set bout ranks.
 *
 * @param $scores
 *    scores from database
 * 
 * @return
 *    Array containing team id, name, rank, and score.
 */
function _scorecreep_parse_bout_scores($scores) {
  $bout_scores = array();
  $bout_ranks = array();
  $teams = array();
  if (!empty($scores)) {
    foreach ($scores as $raw_score) {
      // Sanitize the data before handing it off to the theme layer.
      $score = array_map('check_plain', $raw_score);
      if (!array_key_exists($score['field_performance_team_nid'], $bout_scores)) {
          $bout_scores[$score['field_performance_team_nid']] = array(
            'team_id' => $score['field_performance_team_nid'],
            'score' => $score['field_score_value'],
          );
        $teams[] = $score['field_performance_team_nid'];
      }
      else {
        $bout_scores[$score['field_performance_team_nid']]['score'] +=  (float)$score['field_score_value'];
      }
      if ($score['field_rotation_value'] > NUM_ROTATIONS && $score['field_tiebreakwinner_value'] == 0) {
        $bout_scores[$score['field_performance_team_nid']]['tiebreakloser'] = TRUE;
      }
      else {
        $bout_scores[$score['field_performance_team_nid']]['tiebreakloser'] = FALSE;
      }
    }
    // TODO handle ties
    array_sort_by_subval($bout_scores, "score");
    $team_names = node_load_multiple($teams);
    foreach ($bout_scores as $score) {
      $bout_ranks[] = array(
        'teamid' => $score['team_id'],
        'team' => $team_names[$score['team_id']]->title,
        'rank' => 1,
        'score' => round($score['score'], 1),
        'tiebreakloser' => $score['tiebreakloser'],
      );
    }
  }
  $bout_ranks = _scorecreep_get_bout_rank($bout_ranks);

  usort($bout_ranks, "array_sort_by_rank");

  return $bout_ranks;
}

/**
 * Get the rank of a team from an array.
 * @param {object} team object
 * @param {array} object sorted by score
 * @return {integer} rank
 */
function _scorecreep_get_bout_rank($scores) {
  $rank = 0;
  $prev_val = -1;
  for($i = 0; $i < count($scores); $i++) {
    if ($i == 0 || $prev_val != $scores[$i]['score']) {
      $rank = $i + 1;
    }
    $scores[$i]['rank'] = $rank;
    // handle ties for first
    if ($scores[$i]['tiebreakloser']) {
      $scores[$i]['rank'] = $scores[$i]['rank'] + 1;
    }
    $prev_val = $scores[$i]['score'];
  }
  return $scores;
}

function array_sort_by_rank($a, $b) {
  if ($a['rank'] == $b['rank']) {
    return 0;
  }
  return ($a['rank'] < $b['rank']) ? -1 : 1;
}

function _scorecreep_get_tiebreaker(&$team_score) {
  if (array_key_exists('tiebreakwinner', $team_score) && $team_score['tiebreakwinner'] == 0) {
    return 1;
  }
  return 0;
}

/**
 * Helper function to sort objects.
 */
function array_sort_by_subval(&$array, $key) {
    foreach ($array as &$v) {
        $v['__________'] = $v[$key];
    }
    usort($array, 'sort_by_underscores');
    foreach ($array as &$v) {
        unset($v['__________']);
    }
}

/**
 * Helper function to sort objects.
 */
function sort_by_underscores($a, $b) {
    if ($a['__________'] == $b['__________']) return 0;
    if ($a['__________'] > $b['__________']) return -1;
    return 1;
}

/**
 * Load team nodes.
 */
function _scorecreep_get_team_names(&$teams) {
  $result = db_select('node', 'n')
    ->fields('n', array('title'))
    ->condition('nid', $teams)
    ->execute();
  return $result->fetchAllAssoc('nid');
}

function _scorecreep_get_bout_scores($bout_id) {
  // should just load nodes by taxonomy
  $scores = '';
  $query = db_select('field_data_field_score', 'fs');
  $query->leftJoin('field_data_field_performance_team', 'pt', 'pt.entity_id = fs.entity_id');
  $query->leftJoin('field_data_field_bout', 'fb', 'fb.entity_id = fs.entity_id');
  $query->leftJoin('field_data_field_tiebreakwinner', 'ftb', 'ftb.entity_id = fs.entity_id');
  $query->leftJoin('field_data_field_rotation', 'ffr', 'ffr.entity_id = fs.entity_id');
  $query->addField('pt', 'field_performance_team_nid');
  $query->addField('ftb', 'field_tiebreakwinner_value');
  $query->addField('ffr', 'field_rotation_value');
  $query->addField('fs', 'field_score_value');
  $query->condition("fb.field_bout_tid", $bout_id);  
  $scores = $query->execute()->fetchAll(PDO::FETCH_ASSOC);

  return $scores;
}
/**
 * Get team ranks.
 */
function _scorecreep_get_ranks_for_team(&$team) {
  $ranks = array();
  $terms = array();
  $nodes = array();
  $bouts = array();

  // get bouts this team is associated with (via taxonomy)
  foreach ($team->field_team_bouts['und'] as $bout) {
    $terms[] = $bout['tid'];
  }
  for ($i = 0; $i < count($terms); $i++) {
    //$nodes = array_merge($nodes, (array)taxonomy_select_nodes($terms[$i]));
    $x = taxonomy_term_load($terms[$i]);
    //print_r($x);
    $ranks[$x->name] = _scorecreep_parse_bout_scores(_scorecreep_get_bout_scores($terms[$i]));
  }
/*
  $bouts = node_load_multiple($nodes);
  //print_r($bouts);

  // get scores and ranks for each bout
  foreach ($bouts as $bout) {
      print_r($bout->type); 
    if ($bout->type == 'scorecreep_bout') {
      //$ranks[$bout->nid] = _scorecreep_parse_bout_scores(_scorecreep_get_bout_scores($bout->nid));
      $ranks[$bout->nid] = _scorecreep_parse_bout_scores(_scorecreep_get_bout_scores($bout->field_bout['und'][0]['tid']));
    }
  }
*/
  $team_ranks = array();
  //for ($i = 0; $i < count($ranks); $i++) {
  foreach ($ranks as $key => $rank) {
    for ($j = 0; $j<count($rank); $j++) {
      if ($rank[$j]['team'] == $team->title) {
        $team_ranks[] = array(
          'bout' => $key,
          'rank' => $rank[$j]['rank'],
          'score' => $rank[$j]['score'],
        );
      }
    }
  }
  return $team_ranks;
}

function _scorecreep_get_teams_in_bout($tid, &$teams) {
  $nids = (array)taxonomy_select_nodes($tid, FALSE);
  $teams_in_bout = array();
  for ($i = 0; $i < count($nids); $i++) {
    foreach( $teams as $team) {
      if ($team->nid == $nids[$i]) {
        $teams_in_bout[] = $team->nid;
      }
    }
  }
  return $teams_in_bout;
}

function _scorecreep_get_all_opponents(&$team, &$teams) {
  $opponents = array();
  $teams_in_bout = array();
  // get all the teams in both bouts
  foreach ($team->field_team_bouts['und'] as $bout) {
    $teams_in_bout = array_merge(_scorecreep_get_teams_in_bout($bout['tid'], $teams), $teams_in_bout);
  }

  for ($i = 0; $i < count($teams_in_bout); $i++) {
    if ($teams_in_bout[$i] == $team->nid) {
      continue;
    }
    $opponents[] = $teams_in_bout[$i];
  }
  // get nids for all the nodes in the bouts
  // this might not work with an array?
  // $nids = (array)taxonomy_select_nodes($terms, FALSE); 
  // go through the nids
  return $opponents;
}

function _scorecreep_get_relative_strength(&$teams) {
  foreach ($teams as $id => $team) {
    $ranks = 0;
    $opponents = _scorecreep_get_all_opponents($team, $teams); 
    for ($i = 0; $i < count($opponents); $i++) {
      $ranks += $teams[$opponents[$i]]->scorecreep_total_rank;  
    }
    if (count($opponents) > 0) { 
      $team->scorecreep_relative_strength = number_format($ranks/count($opponents), 2);
    }
  }
}

function sort_total_ranks($ranks) {
  $rank_list = array();
  foreach ($ranks as $key => $row) {
    $rank[$key]  = $row['total_rank'];
    $strength[$key] = $row['relative_strength'];
    $score[$key] = $row['total_score'];
  }

  // Sort the data with rank ascending, score descending
  // Add $ranks as the last parameter, to sort by the common key
  array_multisort($rank, SORT_ASC, $strength, SORT_ASC, $score, SORT_DESC, $ranks);
  // we are sorted add the ranking numbers as set a new array in 
  // the order we want to display
  $count = 1; 
  foreach ($ranks as  $rank) {
    $rank_list[] = array(
      'place' => $count++,
      'title' => $rank['title'],
      'rank0' => $rank['rank0'],
      'score0' => $rank['score0'],
      'rank1' => $rank['rank1'],
      'score1' => $rank['score1'],
      'total_rank' => $rank['total_rank'],
      'relative_strength' => $rank['relative_strength'],
      'total_score' => $rank['total_score']
    );
  }

  return $rank_list;
}

function _scorecreep_get_ranks_for_all_teams($tourney_id) {
  // get all team nids
  $nids = (array)taxonomy_select_nodes($tourney_id, FALSE);
  $nodes = node_load_multiple($nids);
  $teams = array();
  $ranks = array();
  //$nodes = (array)taxonomy_select_nodes($tid[0]);
  //print_r($nodes);

  // loop though team array and set ranks for each
  if (isset($nodes)) {
    foreach ($nodes as $node) {
      if ($node->type == 'scorecreep_team') {
        $team = $node;
        $team->scorecreep_ranks = _scorecreep_get_ranks_for_team($team);
        $team->scorecreep_total_rank = null;
        $team->scorecreep_total_score = 0;
        $team_rank['title'] = $team->title;

        // get total rank
        for ($i = 0; $i < count($team->scorecreep_ranks); $i++) {
          $rank_label = 'rank' . $i; 
          $score_label = 'score' . $i; 
          $team_rank[$rank_label] = $team->scorecreep_ranks[$i]['rank'];
          $team_rank[$score_label] = $team->scorecreep_ranks[$i]['score'];
          $team->scorecreep_total_rank += $team->scorecreep_ranks[$i]['rank']; 
          $team->scorecreep_total_score += $team->scorecreep_ranks[$i]['score']; 
        }
        //print $team->title . " rank: " . $team->scorecreep_total_rank . " score: " . $team->scorecreep_total_score . "<br />";
        $team_rank['total_rank'] = $team->scorecreep_total_rank;
        $team_rank['total_score'] = $team->scorecreep_total_score;
        $teams[$team->nid] = $team;
        $ranks[] = $team_rank;
      }
    }
    // set ranks
    _scorecreep_get_relative_strength($teams);

    for ($i = 0; $i < count($ranks); $i++) {
      foreach ($teams as $team) {
        if ($team->title == $ranks[$i]['title']) {
          $ranks[$i]['relative_strength'] = $team->scorecreep_relative_strength;
        }
      }
    }
    $ranks = sort_total_ranks($ranks);
  }

  // get relative strength

  // return array
  return $ranks;
}

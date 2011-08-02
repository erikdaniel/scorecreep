<?php

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
    foreach ($scores as $entry) {
      // Sanitize the data before handing it off to the theme layer.
      $score = array_map('check_plain', $entry);
      if (!array_key_exists($score['field_performance_team_nid'], $bout_scores)) {
        $bout_scores[$score['field_performance_team_nid']] = array(
          'team_id' => $score['field_performance_team_nid'],
          'score' => $score['field_score_value'],
        );
        $teams[] = $score['field_performance_team_nid'];
      }
      else {
        $bout_scores[$score['field_performance_team_nid']]['score'] +=  $score['field_score_value'];
      }
    }
    // TODO handle ties
    array_sort_by_subval($bout_scores, "score");
    $team_names = node_load_multiple($teams);
    $i = 1;
    foreach ($bout_scores as $score) {
      $bout_ranks[] = array(
        'teamid' => $score['team_id'],
        'team' => $team_names[$score['team_id']]->title,
        'rank' => $i++,
        'score' => $score['score'],
      );
    }
  }
  return $bout_ranks;
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
  $query->addField('pt', 'field_performance_team_nid');
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

function _scorecreep_get_teams_in_bout($tid) {
  $nids = (array)taxonomy_select_nodes($tid, FALSE);
  // loop through this and check if these nodes are teams or not 

}

function _scorecreep_get_all_opponenents(&$team) {
  // get all the bouts
  foreach ($team->field_team_bouts['und'] as $bout) {
    $terms[] = $bout['tid'];
  }
  // get all the teams in the bouts that are not this team
  // using _scorecreep_get_teams_in_bout()
}

function sort_total_ranks($ranks) {
  foreach ($ranks as $key => $row) {
    $rank[$key]  = $row['total_rank'];
    $score[$key] = $row['total_score'];
  }

// Sort the data with volume descending, edition ascending
// Add $data as the last parameter, to sort by the common key
  array_multisort($rank, SORT_ASC, $score, SORT_DESC, $ranks);
  return $ranks;
}

function _scorecreep_get_ranks_for_all_teams($tourney_id) {
  // get all team nids
  $nids = (array)taxonomy_select_nodes($tourney_id, FALSE);
  $nodes = node_load_multiple($nids);
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
        $ranks[] = $team_rank;
      }
      
    }

    $ranks = sort_total_ranks($ranks);
  }

  // get relative strength

  // return array
  return $ranks;
}

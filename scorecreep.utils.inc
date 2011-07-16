<?php

/** 
 * Callback to return the node id from a given string.
 *
 * @param $str
 *  A bout node id OR bout node alias.
 */
function bout_id_or_alias_load($str) {
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
  print_r($teams);
  $result = db_select('node', 'n')
    ->fields('n', array('title'))
    ->condition('nid', $teams)
    ->execute();
  return $result->fetchAllAssoc('nid');
}

function _scorecreep_get_bout_scores($bout_id) {
  $scores = '';
  $query = db_select('field_data_field_score', 'fs');
  $query->leftJoin('field_data_field_performance_team', 'pt', 'pt.entity_id = fs.entity_id');
  $query->leftJoin('field_data_field_perf_bout', 'fb', 'fb.entity_id = fs.entity_id');
  $query->addField('pt', 'field_performance_team_nid');
  $query->addField('fs', 'field_score_value');
  $query->condition("fb.field_perf_bout_value", $bout_id);  
  $scores = $query->execute()->fetchAll(PDO::FETCH_ASSOC);

  return $scores;
}
/**
 * Get team ranks.
 */
function _scorecreep_get_ranks_for_team($team_id) {
  $ranks = array();


  return $ranks;
}

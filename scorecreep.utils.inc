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
 * Callback to return the bout term id from a given string.
 */
function bout_term_id_load($str) {
  $term = false; 
  $bout_term_id = false;
  $arr = explode('-', $str);
  $tournament = array_shift(array_values(taxonomy_get_term_by_name($arr[0])));
  $nps_bout_terms = taxonomy_get_children($tournament->tid);
  if (is_numeric($arr[1])) {
    foreach ($nps_bout_terms as $term) {
      $b = "Bout " . $arr[1];
      if ($term->name == $b) {
        $bout_term_id = $term->tid;
      }
    }
  }
  return $bout_term_id;
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

function _scorecreep_get_performance_ranks($scores) {
  $rank = 0;
  $prev_val = -1;
  for($i = 0; $i < count($scores); $i++) {
    if ($i == 0 || $prev_val != $scores[$i]['score']) {
      $rank = $i + 1;
    }
    $scores[$i]['rank'] = $rank;

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


function array_sort_by_score($a, $b) {
  if ($a['score'] == $b['score']) {
    return 0;
  }
  return ($a['score'] > $b['score']) ? -1 : 1;
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

function _scorecreep_get_teams_in_bout_by_tid($tid) {
 $query = new EntityFieldQuery();

  $query
    ->entityCondition('entity_type', 'node', '=')
    ->propertyCondition('type', 'scorecreep_team', '=')
    ->fieldCondition('field_team_bouts', 'tid', $tid, '=');
  $arr = $query->execute();
  return $arr['node'];

}

function _scorecreep_get_bout_node_id_from_tid($tid) {
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'node', '=')
    ->propertyCondition('type', 'scorecreep_bout', '=')
    ->fieldCondition('field_bout', 'tid', $tid, '=');
  $arr = $query->execute();
  return  array_shift(array_values($arr['node']));
}

function _scorecreep_get_performers_from_team_id(&$team) {
  $poets = array();
  if (isset($arr['user']) ) {
    $poets = $arr['user'];
  }
   $poets = array();
  $query = db_select('field_data_field_user_teams', 'tm');
  $query->leftJoin('node', 'n', 'tm.field_user_teams_nid = n.nid');
  $query->addField('tm', 'entity_id');
  $query->addField('tm', 'field_user_teams_nid');
  $query->addField('n', 'title');
  $query->condition("tm.field_user_teams_nid", $team, 'IN');  
  $poets = $query->execute()->fetchAllAssoc('entity_id');//(PDO::FETCH_ASSOC);


  return  $poets;//array_shift(array_values($arr['user']));
}

function _scorecreep_get_performances_in_bout($bout_id) {
 $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'node', '=')
    ->propertyCondition('type', 'scorecreep_performance', '=')
    ->fieldCondition('field_bout', 'tid', $bout_id, '=', 0);
  $val = $query->execute();
  $perfs = array();
  if (isset($val['node']) ){
    $perfs = $val['node'];
  }
  return $perfs;
}

function _scorecreep_get_performances_by_rank($bout_id) {
 $query = new EntityFieldQuery();

  $query
    ->entityCondition('entity_type', 'node', '=')
    ->propertyCondition('type', 'scorecreep_performance', '=')
    ->fieldCondition('field_rotation', 'value', NUM_ROTATIONS, '<=', 0);
  if ($bout_id) {
    $query->fieldCondition('field_bout', 'tid', $bout_id, '=', 0);
  }

  $result = $query->execute();

  $team_ids = array();
  $poet_ids = array(); 
  $scores = array();

  $pids = array();
  if ( array_key_exists('node', $result) ) {
    foreach($result['node'] as $record) {
      $pids[] = $record->nid;
    }
  }
  $performances = entity_load('node', $pids);   
  
  foreach ($performances as $perf) {
    //dvm($perf);
    $team_id = $perf->field_performance_team['und'][0]['nid'];
    if (!in_array($team_id, $team_ids)) { 
      $team_ids[] = $team_id; 
    }

    if (!empty($perf->field_performance_performer)) {
      $arr = $perf->field_performance_performer['und'];
      foreach ($arr as $p) {
        if (!in_array($p['uid'], $poet_ids)) {
          $poet_ids[] = $p['uid'];
        }
      }
    }
  }
  $poets = user_load_multiple($poet_ids);
  $teams = node_load_multiple($team_ids);

  foreach ($performances as $perf) {
    $team_id = $perf->field_performance_team['und'][0]['nid'];
    $scores[] = array(
      'rank' => 0,
      'team_name' => $teams[$team_id]->title,
      'performer' => _scorecreep_get_performers_in_performance($perf->field_performance_performer, $poets),
      'group' => ($perf->field_group_piece['und'][0]['value'] == 1) ? t("Yes") : '',
      'rotation' => $perf->field_rotation['und'][0]['value'],
      'score' => $perf->field_score['und'][0]['value'],
      'bout_term' => $perf->field_bout['und'][0]['tid'],
    );
  }

  // sort by score high to low
  usort($scores, "array_sort_by_score");
  // set ranks
  $scores = _scorecreep_get_performance_ranks($scores);

  return $scores;
}

function _scorecreep_get_performers_in_performance($perf, $poets) {
  $poetnames = array();
  if (empty($poets) || empty($perf)) {
    return '';// this is an error
  }
  $a = $perf['und'];
  for ($i = 0; $i < count($a); $i++) {
    $poetnames[] = $poets[$a[$i]['uid']]->field_stage_name['und'][0]['safe_value'];
  }
  return implode(', ', $poetnames);
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
  $team_ranks = array();
  //for ($i = 0; $i < count($ranks); $i++) {
  foreach ($ranks as $key => $rank) {
    for ($j = 0; $j<count($rank); $j++) {
      if ($rank[$j]['team'] == $team->title) {
        $team_ranks[] = array(
          'bout' => $key,
          'rank' => $rank[$j]['rank'],
          'score' => $rank[$j]['score'],
	  'bid' => intval(str_replace('Bout ', '', $key)),
        );
      }
    }
  }
  $bid = array();
 foreach ($team_ranks as $key => $row) {
    $bid[$key]  = $row['bid'];
 }
  
 array_multisort($bid, SORT_ASC,  $team_ranks);
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
  $stren = array();
  foreach ($ranks as $key => $row) {
    $rank[$key]  = $row['total_rank'];
    $strength[$key] = $row['relative_strength'];
    $score[$key] = $row['total_score'];
    $strength_flag = (isset($row['relative_strength']) && $row['relative_strength'] > 0);
    $stren[] = $row['relative_strength'];
  }

  // Sort the data with rank ascending, score descending
  // Add $ranks as the last parameter, to sort by the common key
  if(!empty($ranks)) {
    if(count($stren) > 74) {//TODO fix this
      array_multisort($rank, SORT_ASC, $strength, SORT_ASC, $score, SORT_DESC, $ranks);
    }
    else {
      array_multisort($rank, SORT_ASC, $score, SORT_DESC, $ranks);
    }
  }
  // we are sorted add the ranking numbers as set a new array in 
  // the order we want to display
  $count = 1; 
  foreach ($ranks as  $rank) {
    //if (array_key_exists('rank0', $rank)) {
      $rank_list[] = array(
        'place' => $count++,
        'title' => $rank['title'],
        'rank0' => isset($rank['rank0']) ? $rank['rank0'] : '',
        'score0' => isset($rank['score0']) ? $rank['score0']: '',
        'rank1' => isset($rank['rank1']) ? $rank['rank1'] : '',
        'score1' => isset($rank['score1']) ? $rank['score1'] : '',
        'total_rank' => $rank['total_rank'],
        'relative_strength' => (isset($rank['rank0']) && isset($rank['rank1']))? $rank['relative_strength'] : '',
        'total_score' => $rank['total_score'],
      );
    //}
  }
  return $rank_list;
}

function _scorecreep_get_ranks_for_all_teams($tourney_id) {
  // get all team nids
  $nids = (array)taxonomy_select_nodes($tourney_id, FALSE);
  $nodes = node_load_multiple($nids);
  $teams = array();
  $ranks = array();

  // loop though team array and set ranks for each
  if (isset($nodes)) {
    foreach ($nodes as $node) {
      if ($node->type == 'scorecreep_team') {
	$team_rank = array();
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
        $team_rank['total_rank'] = $team->scorecreep_total_rank;
        $team_rank['total_score'] = $team->scorecreep_total_score;
        $teams[$team->nid] = $team;
        if (isset($team_rank['total_rank'])) {
        $ranks[] = $team_rank;
	}
      }
    }
    // set ranks
    _scorecreep_get_relative_strength($teams);

    for ($i = 0; $i < count($ranks); $i++) {
      foreach ($teams as $team) {
        if ($team->title == $ranks[$i]['title']) {
          $ranks[$i]['relative_strength'] =  $team->scorecreep_relative_strength;
        }
      }
    }
    $ranks = sort_total_ranks($ranks);
  }

  // get relative strength

  // return array
  return $ranks;
}


function _scorecreep_get_ranks_for_all_group_pieces($term) {
  $nps_bout_terms = taxonomy_get_children($term);
  $all_perfs = array();
  foreach ($nps_bout_terms as $bout_term) {
     $all_perfs = array_merge( _scorecreep_get_performances_by_rank($bout_term->tid), $all_perfs); 
  }
  usort($all_perfs, "array_sort_by_rank");
  $group_perfs = array();
  for ($i = 0; $i < count($all_perfs); $i++) {
    if ($all_perfs[$i]['group'] == 'Yes') {
      $group_perfs[] = array(
        'rank' => $all_perfs[$i]['rank'],
        'team_name' => $all_perfs[$i]['team_name'],
        'bout' => $nps_bout_terms[$all_perfs[$i]['bout_term']]->name,
        'rotation' => $all_perfs[$i]['rotation'],
        'score' => $all_perfs[$i]['score'],
      );
    }

  }
  _scorecreep_group_piece_final_sort($group_perfs);
  return $group_perfs;
}

function _scorecreep_group_piece_final_sort(&$group_pieces) {
	foreach ($group_pieces as $key => $row) {
	    $rank[$key]  = $row['rank'];
	    $score[$key] = $row['score'];
	  }

	if($group_pieces) {
	    array_multisort($rank, SORT_ASC, $score, SORT_DESC, $group_pieces);
	}
}

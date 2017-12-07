<?php

/**
 * @file
 * Batch functionality for updating Handle URLs.
 */

/**
 * Batch operation that updates a Handle URL.
 *
 * @param array $mappings
 *   An array keyed by a content model, containing the content model and
 *   datastream that a Handle is configured for.
 * @param DrushBatchContext $context
 *   Context of the batch operation.
 */
function sdsudora_update_handle_batch_operation(array $mappings, DrushBatchContext &$context) {
  module_load_include('inc', 'islandora_handle', 'includes/handle');
  $sandbox =& $context['sandbox'];
  if (!isset($sandbox['total'])) {
    $sandbox['progress'] = 0;
    $qp = new IslandoraSolrQueryProcessor();
    sdsudora_set_handle_batch_qp($qp, $mappings, 1000000);
    $qp->executeQuery(FALSE);
    $sandbox['total'] = $qp->islandoraSolrResult['response']['numFound'];
    if ($sandbox['total'] == 0) {
      $context['finished'] = 1;
      $context['message'] = t('Nothing to update.');
      return;
    }
    $sandbox['result_stash'] = array();
  }
  if (empty($sandbox['result_stash'])) {
    $limited_qp = new IslandoraSolrQueryProcessor();
    sdsudora_set_handle_batch_qp($limited_qp, $mappings, 1000);
    if (isset($sandbox['last_date'])) {
      $limited_qp->solrParams['fq'][] = format_string('fgs_createdDate_dt:{!last_date TO *} OR (fgs_createdDate_dt:"!last_date" AND PID_kw:{!last_pid TO *})', array(
        '!last_date' => $sandbox['last_date'],
        '!last_pid' => $sandbox['last_pid'],
      ));
    }
    drush_log(dt('Querying for 1000 objects.'));
    $limited_qp->executeQuery(FALSE);
    if ($limited_qp->islandoraSolrResult['response']['numFound'] == 0) {
      $context['finished'] = 1;
      $context['message'] = t('Updated @count PID(s) and ran out of items early, Solr may have gone away.', array(
        '@count' => $sandbox['progress'],
      ));
    }
    $sandbox['result_stash'] = $limited_qp->islandoraSolrResult['response']['objects'];
  }
  $result = array_shift($sandbox['result_stash']);
  $sandbox['last_date'] = $result['solr_doc']['fgs_createdDate_dt'];
  $sandbox['last_pid'] = $result['solr_doc']['PID'];
  $solr_doc = $result['solr_doc'];
  $object = islandora_object_load($solr_doc['PID']);

  // When Handles are inserted they are rightfully URL-encoding the PID.
  // For searching purposes replace this here.
  $encoded_pid = urlencode($object->id);
  $suffix = islandora_handle_construct_suffix($object);
  $prefix_and_suffix = format_string('!prefix/!suffix', array(
    '!prefix' => variable_get('islandora_handle_server_prefix', '1234567'),
    '!suffix' => $suffix,
  ));
  $location_response = islandora_handle_get_handle_location($prefix_and_suffix);
  if (isset($location_response->headers['location'])) {
    $bad_url = format_string("http://default/islandora/object/!pid", array(
      '!pid' => $encoded_pid,
    ));
    if ($location_response->headers['location'] == $bad_url) {
      $new_target = url("islandora/object/$suffix", array(
        'language' => (object) array('language' => FALSE),
        'absolute' => TRUE,
      ));
      drush_log($new_target);
      islandora_handle_update_handle_url($prefix_and_suffix, $new_target);
      drush_log(dt('Updated the Handle location for @pid.', array(
        '@pid' => $object->id,
      )));
    }
    else {
      drush_log(dt('The Handle location (@location) is not pointing to a default URL for @pid.', array(
        '@pid' => $object->id,
        '@location' => $location_response->headers['location'],
      )));
    }
  }
  else {
    drush_log(dt('A Handle for @pid does not exist, doing nothing.', array(
      '@pid' => $object->id,
    )));
  }
  $sandbox['progress'] = min($sandbox['total'], $sandbox['progress'] + 1);
  $context['finished'] = $sandbox['progress'] / $sandbox['total'];
  $context['message'] = t('Updated @progress of @total.', array(
    '@progress' => $sandbox['progress'],
    '@total' => $sandbox['total'],
  ));
}

/**
 * Helper to add some common things to the QP for the batch.
 *
 * @param IslandoraSolrQueryProcessor $qp
 *   The QueryProcessor object to be modified.
 * @param array $mappings
 *   An array keyed by a content model, containing the content model and
 *   datastream that a Handle is configured for.
 * @param string $solr_limit
 *   The number of records to query for.
 */
function sdsudora_set_handle_batch_qp(IslandoraSolrQueryProcessor $qp, array $mappings, $solr_limit) {
  $cmodel_filters = array();
  $datastream_filters = array();
  foreach ($mappings as $result) {
    $cmodel_filters[] = format_string('"info:fedora/!cmodel"', array('!cmodel' => $result['cmodel']));
    $datastream_filters[] = format_string('"!datastream"', array('!datastream' => $result['datastream']));
  }
  $datastream_filters = array_unique($datastream_filters);
  $qp->buildQuery('*:*');
  $qp->solrParams['facet'] = 'false';
  $qp->solrParams['fq'] = array();
  $qp->solrParams['fq'][] = format_string('!cmodel_field:(!cmodels)', array(
    '!cmodel_field' => variable_get('islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms'),
    '!cmodels' => implode(' OR ', $cmodel_filters),
  ));
  $qp->solrParams['fq'][] = format_string('!datastream_field:(!datastreams)', array(
    '!datastream_field' => variable_get('islandora_solr_datastream_id_field', 'fedora_datastreams_ms'),
    '!datastreams' => implode(' OR ', $datastream_filters),
  ));
  $qp->solrParams['facet'] = 'false';
  $qp->solrParams['fl'] = 'PID, PID_kw, fgs_createdDate_dt';
  $qp->solrParams['sort'] = 'fgs_createdDate_dt asc, PID_kw asc';
  $qp->solrLimit = $solr_limit;
}

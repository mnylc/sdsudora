<?php

/**
 * @file
 * Drush hook implementations for this module.
 */

/**
 * Implements hook_drush_command().
 */
function sdsudora_drush_command() {
  $commands = array();

  $commands['sdsudora-import-raw_qdc'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
    'description' => dt('Import a DSpace AIP export.'),
    'drupal_dependencies' => array('islandora', 'islandora_batch'),
    'examples' => array('drush -u 1 -v sdsudora-import-raw_qdc --file="/some_dir/mets.xml"'),
    'options' => array(
      'file' => array(
        'description' => dt('file holding metadata description.'),
      ),
    ),
  );
  $commands['sdsudora-update-handle-urls'] = array(
    'description' => dt('Updates Handle URLs as the --uri option may not have been set.'),
    'drupal_dependencies' => array('islandora', 'sdsudora'),
    'examples' => array(
      'drush -u 1 -v sdsudora-update-handles-urls --uri=https://digitallibrary.sdsu.edu/',
    ),
  );
  return $commands;
}

/**
 * Drush command callback to create and ingest MODS from QDC.
 */
function drush_sdsudora_import_raw_qdc() {
  module_load_include('inc', 'sdsudora', 'includes/utilities');
  $object_info = sdsudora_extract_data_from_mets(drush_get_option('file'));

  $islandora_pid = db_select('dspace_aip_import_handles')
    ->fields('dspace_aip_import_handles', array('pid'))
    ->condition('handle', $object_info['handle'], '=')
    ->execute()->fetchField();

  $object = islandora_object_load($islandora_pid);
  if ($object && !isset($object['MODS'])) {
    drush_log(dt("Generating MODS for @pid", array('@pid' => $islandora_pid)), 'success');
    $mods = sdsudora_generate_mods($object_info['metadata']);
    sdsudora_add_metadata($object, $mods);
  }
  else {
    drush_log(dt("MODS exists for @pid, skipping", array('@pid' => $islandora_pid)), 'warning');
  }
}

/**
 * Drush command callback to update Handle URLs.
 */
function drush_sdsudora_update_handle_urls() {
  $module = drupal_get_path('module', 'sdsudora');
  $mappings = db_select('islandora_handle_apply_handle', 'h')
    ->fields('h', array('cmodel', 'datastream'))
    ->execute()
    ->fetchAllAssoc('cmodel', PDO::FETCH_ASSOC);
  batch_set(array(
    'title' => dt('Updating Handle URLs'),
    'operations' => array(
      array('sdsudora_update_handle_batch_operation', array($mappings)),
    ),
    'file' => "$module/includes/update_handles.batch.inc",
  ));
  drush_backend_batch_process();
}

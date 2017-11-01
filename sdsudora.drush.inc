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
  if ($object) {
    $mods = sdsudora_generate_mods($object_info['metadata']);
    sdsudora_add_metadata($object, $mods);
  }
}

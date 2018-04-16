<?php

/**
 * @file
 * Breadcrumb specific functions for the module.
 */


/**
 * Alters breadcrumbs used on Solr search results and within Islandora views.
 *
 * @param array $breadcrumbs
 *   Breadcrumbs array to be altered by reference. Each element is markup.
 * @param string $context
 *   Where the alter is originating from for distinguishing.
 * @param AbstractObject $object
 *   (Optional) AbstractObject representing object providing breadcrumb path
 */
function sdsudora_islandora_breadcrumbs_alter(&$breadcrumbs, $context, $object = NULL) {
  if ($context == 'islandora') {
    $values = variable_get('sdsudora_settings', array());
    if (count($values) && isset ($values['sdsudora_settings']['map'])) {
      $i=0;
      dpm($breadcrumbs);
      foreach($values['sdsudora_settings']['map'] as $key => $value) {
      
      $i++; 
      }
    }
    
  }

}
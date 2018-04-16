<?php

/**
* @file
* Admin setting form file for SDSUdora.
* @author Diego Pino Navarro
*/

/**
 * The configuration form for the SDSUdora module.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the configuration form.
 */
function sdsudora_admin_form($form, &$form_state) {
  $values = variable_get('sdsudora_settings', array());
  $existingvalues = count($values);
  $numvalues = $existingvalues == 0 ? 1 : $existingvalues;
  $max = 5;
  $numberoffields = $numvalues > $max ? $max : $numvalues;
  $form['sdsudora_settings'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('SDSUdora breadcrumb mappings'),
  );

  for ($i=0; $i<$numberoffields; $i++) {
    $form['sdsudora_settings']['sdsudora_settings_o_'. $i] = array(
      '#type' => 'textfield',
      '#title' => t('Object to be replaced'),
      '#value' => isset($values['sdsudora_settings']['sdsudora_settings_o_'. $i]) ? $values['sdsudora_settings']['sdsudora_settings_o_'. $i] : 'islandora:root',
    );
    $form['sdsudora_settings']['sdsudora_settings_d_'. $i] = array(
      '#type' => 'textfield',
      '#title' => t('Replacement URI'),
      '#value' => isset($values['sdsudora_settings']['sdsudora_settings_d_'. $i]) ? $values['sdsudora_settings']['sdsudora_settings_d_'. $i] : '/islandora',
    );
  }

  return system_settings_form($form);
}

/**
 * Validation for the sdsudora_admin_form configuration form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function sdsudora_admin_form_validate($form, &$form_state) {
  $i=0;
  dpm($form_state['values']['sdsudora_settings']);
  foreach($form_state['values']['sdsudora_settings'] as $key => $value) {
    if ($key == 'sdsudora_settings_o_'. $i) {
      if (!islandora_is_valid_pid($value)) {
        form_set_error('sdsudora_settings][sdsudora_settings_o_'. $i, t('The PID @field is not a valid Islandora Object one', array('@field' => $value)));
      }
    }
    if ($key == 'sdsudora_settings_d_'. $i) {
     if (!drupal_valid_path($value, FALSE)) {
       form_set_error('sdsudora_settings][sdsudora_settings_d_'. $i, t('The Path @field is not a valid Drupal path for your site or you don\'t have permissions to access it', array('@field' => $value)));
     }
    }
  $i++;  
  }
}
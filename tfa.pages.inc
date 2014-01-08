<?php

/**
 * @file tfa.pages.inc
 */

function tfa_begin_form($uid) {
  $account = user_load($uid);

  return drupal_get_form('tfa_form', $account);
}

function tfa_form($form, $form_state, $account) {

  // Check validation flood.
  /*if (!flood_is_allowed('tfa_validate', variable_get('tfa_hourly_threshold', 5))) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again later.'), 'error');
    return drupal_access_denied();
  }*/ // @todo

  if (empty($form_state['storage']['tfa_context'])) {
    $tfa = tfa_get_process($account);
  }
  else {
    tfa_set_context($account, $form_state['storage']['tfa_context']);
    $tfa = tfa_get_process($account);
  }
  $tfa->begin();
  $form = $tfa->getForm();
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );
  if ($tfa->hasFallback) {
    $form['fallback'] = array(
      '#type' => 'submit',
      '#value' => t("Can't access your account?"),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );
  }

  return $form;
}

function tfa_form_validate($form, &$form_state) {
  // No validation when issuing fallback.
  if (isset($form_state['values']['fallback']) && $form_state['values']['op'] === $form_state['values']['fallback']) {
    return;
  }
//  dpm($form_state, 'validate');
  $code = $form_state['values']['code'];
  $account = user_load($form_state['values']['uid']);

  $tfa = tfa_get_process($account);
  if (!$tfa->validate($code)) {
    form_set_error('code', t('Invalid code.'));
    // Register failure for purposes of flood control.
    flood_register_event('tfa_validate'); // @todo
  }
}

function tfa_form_submit($form, &$form_state) {
  // @todo can form uid value be manipulated??
  $account = user_load($form['uid']['#value']);
  $tfa = tfa_get_process($account);
  $tfa->submitForm($form_state);

 // dpm($tfa->getContext(), 'submit handler');
  if (!$tfa->processComplete()) {
    //drupal_set_message('rebuild');
    // set TFA context
    $form_state['storage']['tfa_context'] = $tfa->getContext();
    $form_state['rebuild'] = TRUE;
  }
  else {
    $tfa->finalize();
    tfa_login($account);
    $form_state['redirect'] = 'user';
  }
}

/**
 * Admin settings form.
 */
function tfa_admin_settings() {
  $form = array();

  // Setup plugins.
  $send_plugins = $totp_plugins = array();
  foreach (module_invoke_all('tfa_api') as $key => $data) {
    if (in_array('TfaSendPlugin', class_implements($data['class']))) {
      $send_plugins[$key] = $data['name'];
    }
    elseif (in_array('TfaTotpPlugin', class_implements($data['class']))) {
      $totp_plugins[$key] = $data['name'];
    }
  }


  $form['configure'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available plugins'),
  );
  if (!empty($send_plugins)) {
    if (count($send_plugins) > 1) {
      $form['configure']['tfa_send'] = array(
        '#type' => 'select',
        '#title' => t('TFA send plugin'),
        '#options' => $send_plugins,
      );
    }
    else {
      $form['configure']['send_none'] = array(
        '#value' => 'markup',
        '#markup' => t('No available TFA Send plugins'),
      );
    }
  }
  if (!empty($totp_plugins)) {
    if (count($totp_plugins) > 1) {
      $form['configure']['tfa_totp'] = array(
        '#type' => 'select',
        '#title' => t('TOTP plugin'),
        '#options' => $totp_plugins,
      );
    }
    else {
      $form['configure']['totp_none'] = array(
        '#value' => 'markup',
        '#markup' => t('No available TFA TOTP plugins'),
      );
    }
  }


  if (count($validate_plugins) > 1) {
    $form['tfa_fallback'] = array(
      '#type' => 'select',
      '#title' => t('Send'),
      '#options' => $send_plugins,
    );
  }

  // If the channel does not define a 'address callback' method default to field.
  // Controlled via Form API #states.
  /*$form['tfa_phone_field'] = array(
    '#type' => 'container',
    '#children'  => t('<div class="error messages">A phone field is required for the TFA process. Add a user field for phone number storage to continue. Consult the <a href="!url">help documentation for more info</a>.</div>', array('!url' => url('admin/help/tfa'))),
    '#states' => array(
      'visible' => array(
        ':input[name="tfa_channel"]' => array('value' => 'sms'),
      )
    ),
  );
  $instances = field_info_instances('user');
  if (!empty($instances['user'])) {
    //drupal_set_message(t('A phone field is required for the TFA process. Add a user field for phone number storage to continue. Consult the <a href="!url">help documentation for more info</a>.', array('!url' => url('admin/help/tfa'))), 'error');
    $options = array();
    foreach ($instances['user'] as $name => $field) {
      $options[$name] = $field['label'];
    }
    // Change to select field.
    unset($form['tfa_phone_field']['#children']);
    $form['tfa_phone_field']['#type'] = 'select';
    $form['tfa_phone_field']['#default_value'] = variable_get('tfa_phone_field', '');
    $form['tfa_phone_field']['#options'] = $options;
    $form['tfa_phone_field']['#description'] = t('Choose the field that stores phone numbers.');
  }
  $form['tfa_required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Require TFA process'),
    '#default_value' => variable_get('tfa_required', 0),
    '#description' => t('Require TFA to login except for accounts that have permission to "Skip TFA". Note, any account that is not setup for TFA (and is not set to skip TFA) will not be able to login.'),
  );

  $form['tfa_send_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Message'),
    '#default_value' => variable_get('tfa_send_message', 'Login code'),
    '#description' => t('Text to prepend before the TFA login code. Plain text only.'),
  );
  $form['tfa_code_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Code length'),
    '#default_value' => variable_get('tfa_code_length', 6),
    '#description' => t('Length of the TFA login code.'),
  );*/

  return system_settings_form($form);
}

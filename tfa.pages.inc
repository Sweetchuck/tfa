<?php

/**
 * @file tfa.pages.inc
 */

/**
 * Start of TFA process form.
 *
 * @param  string $uid User account ID
 */
function tfa_begin_form($uid) {
  $account = user_load($uid);

  return drupal_get_form('tfa_form', $account);
}

/**
 * Main TFA process form builder.
 *
 * Invokes plugin getForm() and handles multi-step fallback.
 */
function tfa_form($form, $form_state, $account) {

  // Check validation flood.
  if (!flood_is_allowed('tfa_validate', variable_get('tfa_hourly_threshold', 5))) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again later.'), 'error');
    return drupal_access_denied();
  }

  if (empty($form_state['storage']['tfa_context'])) {
    $tfa = tfa_get_process($account);
  }
  else {
    tfa_set_context($account, $form_state['storage']['tfa_context']);
    $tfa = tfa_get_process($account);
  }
  // Begin process.
  $tfa->begin();
  $form = $tfa->getForm($form);
  if ($tfa->hasFallback) {
    $form['fallback'] = array(
      '#type' => 'submit',
      '#value' => t("Can't access your account?"),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );
  }
  // Set required UID element.
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );

  return $form;
}

/**
 * TFA form validation handler.
 *
 * Invokes plugin validateForm() and getErrorMessages().
 */
function tfa_form_validate($form, &$form_state) {
  // No validation when issuing fallback.
  if (isset($form_state['values']['fallback']) && $form_state['values']['op'] === $form_state['values']['fallback']) {
    return;
  }
  $account = user_load($form_state['values']['uid']);
  $tfa = tfa_get_process($account);

  if (!$tfa->validateForm($form_state)) {
    foreach ($tfa->getErrorMessages() as $element => $message) {
      form_set_error($element, $message);
    }
    // Register failure for purposes of flood control.
    flood_register_event('tfa_validate');
  }
}

/**
 * TFA form submission handler.
 *
 * Invokes plugin submitForm() and processComplete() and handles starting
 * multi-step if appropriate.
 */
function tfa_form_submit($form, &$form_state) {
  // @todo can form uid value be manipulated??
  $account = user_load($form['uid']['#value']);
  $tfa = tfa_get_process($account);
  $tfa->submitForm($form_state);

  if (!$tfa->processComplete()) {
    // Store TFA context for form rebuild.
    $form_state['storage']['tfa_context'] = $tfa->getContext();
    $form_state['rebuild'] = TRUE;
    // Clear validation flood.
    flood_clear_event('tfa_validate');
    // @todo handle flood control bypass by swapping plugins -- maybe with tfa_begin?
  }
  else {
    // TFA process is complete so finalize and authenticate user.
    $tfa->finalize();
    tfa_login($account);

    // Handle destination and query parameters.
    $query = drupal_get_query_parameters();
    if (!empty($query['pass-reset-token'])) {
      // If pass-reset-token was set then complete final process interrupted
      // from user_pass_reset().
      watchdog('tfa', 'User %name used one-time login link', array('%name' => $account->name));
      drupal_set_message(t('You have just used your one-time login link. It is no longer necessary to use this link to log in. Please change your password.'));
      // Let the user's password be changed without the current password check.
      $token = drupal_random_key();
      $_SESSION['pass_reset_' . $account->uid] = $token;
      $query['pass-reset-token'] = $token;
    }
    $destination = !empty($form_state['redirect']) ? $form_state['redirect'] : 'node';
    if (!empty($query['destination'])) {
      $destination = $query['destination'];
      unset($query['destination']);
      unset($_GET['destination']);
    }
    $form_state['redirect'] = array(
      $destination,
      array('query' => $query),
    );
  }
}

/**
 * Admin settings form.
 */
function tfa_admin_settings() {
  $form = array();

  // Setup plugins.
  $plugins = $send_plugins = $totp_plugins = $login_plugins = array();

  foreach (module_invoke_all('tfa_api') as $key => $data) {
    dpm($data, $key);
    if (in_array('TfaSendPlugin', class_implements($data['class']))) {
      $send_plugins[$key] = $data['name'];
      $plugins[$key] = $data['name'];
    }
    elseif (in_array('TfaTotpPlugin', class_implements($data['class']))) {
      $totp_plugins[$key] = $data['name'];
    }
    elseif (in_array('TfaLoginPlugin', class_implements($data['class']))) {
      $login_plugins[$key] = $data['name'];
    }
    elseif (is_subclass_of($data['class'], 'TfaBasePlugin')) {
      $plugins[$key] = $data['name'];
    }
  }

  // Force set test plugins. @todo
  variable_set('tfa_login_plugin', 'TfaDeviceAuth');
  variable_set('tfa_validate_plugin', 'TfaTotp');
  variable_set('tfa_fallback_plugins', array('TfaEmail', 'TfaRecoveryCode'));

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


  if (count($plugins) >= 1) {
    $form['tfa_fallback'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Fallback'),
      '#options' => $plugins,
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

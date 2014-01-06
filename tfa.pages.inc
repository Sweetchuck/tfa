<?php

/**
 * @file tfa.pages.inc
 */

/**
 * Form for code entry.
 */
function tfa_form($form, $form_state, $uid, $hash = NULL) {
  // Check flood.
  if (!flood_is_allowed('tfa_validate', variable_get('tfa_hourly_threshold', 5))) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again later.'), 'error');
    return drupal_access_denied();
  }

  $account = user_load($uid);

  // $tfa = tfa_get_
  // $tfa->beginProcess()
  // $form = $tfa->entryForm($form)
  // return $form;

  $form['desc'] = array(
    '#markup' => t('Enter your received login code to continue.'),
  );
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $uid,
  );
  $form['code'] = array(
    '#type' => 'textfield',
    '#title' => t('Code'),
    '#required' => TRUE,
  );
  // Provide option to resend code.
  $form['resend'] = array(
    '#type' => 'fieldset',
    '#title' => t('Resend code'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('If you have not received the code you may resend it. You can only resend the code !count times per hour.', array('!count' => variable_get('tfa_hourly_threshold', 5))),
  );
  $form['login'] = array(
    '#type' => 'submit',
    '#value' => t('Log in'),
  );
  $form['resend']['send'] = array(
    '#type' => 'submit',
    '#value' => t('Resend code'),
    '#limit_validation_errors' => array(),
    '#submit' => array('tfa_resend_code'),
  );

  return $form;
}

/**
 * Validate handler for TFA login form.
 */
function tfa_code_form_validate($form, &$form_state) {
  // Validate code.
  // $tfa = tfa_get_
  // $tfa->validateForm($form_state);
  // if (!$tfa->validEntry()) then form_set_error and flood register
  $code = tfa_get_code($form_state['values']['uid']);
  if (!empty($code) && $code['code'] != $form_state['values']['code']) {
    form_set_error('code', t('Invalid code.'));
    // Register failure for purposes of flood control.
    flood_register_event('tfa_validate');
  }
}

/**
 * Submit handler for TFA login form.
 */
function tfa_code_form_submit($form, &$form_state) {
  // $tfa = tfa_get_
  // $tfa->submitForm($form_state);
  //
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);
  tfa_login($account);
}

/**
 * Resend code.
 */
function tfa_resend_code($form, &$form_state) {
  $account = user_load($form['uid']['#value']);
  $code = tfa_generate_code($account);
  tfa_store_code($account->uid, $code);
  _tfa_tfa_process($account);
}

/**
 * Admin settings form.
 */
function tfa_admin_settings() {
  $form = array();

  // Choose communication channel.
  $channels  = array();
  $hook = 'tfa_api';
  foreach (module_implements($hook) as $module) {
    $function = $module . '_' . $hook;
    if (function_exists($function)) {
      $channel = $function();
      $channels[$module] = $channel;
      $send_methods[$module] = $channel['title'];
    }
  }
  $default_channel = variable_get('tfa_channel', 'sms');
  $form['tfa_channel'] = array(
    '#type' => 'select',
    '#title' => t('Channel'),
    '#options' => $send_methods,
    '#default_value' => $default_channel,
    '#description' => t('Choose the channel to communicate the TFA login code.'),
  );

  // If the channel does not define a 'address callback' method default to field.
  // Controlled via Form API #states.
  $form['tfa_phone_field'] = array(
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
  );

  return system_settings_form($form);
}

<?php

/**
 * @file tfa.pages.inc
 */

/**
 * Form for code entry.
 */
function tfa_code_form($form_state, $uid, $hash = NULL) {
  // Check flood.
  if (!flood_is_allowed('tfa_validate', variable_get('tfa_hourly_threshold', 5))) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again later.'), 'error');
    drupal_access_denied();
    exit;
  }

  $account = user_load(array('uid' => $uid));

  $form['desc'] = array(
    '#value' => t('<div class="messages ok">A message containing the code has been sent. Enter your received login code to continue.</div>'),
  );
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $uid,
  );
  $form['code'] = array(
    '#type' => 'textfield',
    '#title' => t('Code'),
  );
  // Provide option to resend code.
  $form['resend'] = array(
    '#type' => 'fieldset',
    '#title' => t('Resend code'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('If you have not received the code you can resend it. You can only resend the code !count times per hour.', array('!count' => variable_get('tfa_hourly_threshold', 5))),
  );
  $form['resend']['send'] = array(
    '#type' => 'button',
    '#value' => t('Resend code'),
  );

  $form['login'] = array(
    '#type' => 'submit',
    '#value' => t('Log in'),
  );

  return $form;
}

/**
 * Validate handler for TFA login form.
 */
function tfa_code_form_validate($form, &$form_state) {
  // Validate code.
  if ($form_state['clicked_button']['#type'] == 'button') {
    $account = user_load(array('uid' => $form['uid']['#value']));
    $code = tfa_generate_code($account);
    tfa_store_code($account->uid, $code);
    tfa_tfa_process($account);
  }
  if (empty($form_state['values']['code'])) {
    form_set_error('code', t('Code field is required.'));
  }
  $code = tfa_get_code($form_state['values']['uid']);
  if ($code['code'] != $form_state['values']['code']) {
    form_set_error('code', t('Invalid code.'));
    // Register failure for purposes of flood control.
    flood_register_event('tfa_validate');
  }
}

/**
 * Submit handler for TFA login form.
 */
function tfa_code_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];
  $account = user_load(array('uid' => $uid));
  $edit = array();

  $user = $account;
  // Update the user table timestamp noting user has logged in.
  $user->login = time();
  db_query("UPDATE {users} SET login = %d WHERE uid = %d", $user->login, $user->uid);

  // Regenerate the session ID to prevent against session fixation attacks.
  sess_regenerate();
  // Mark code as accepted to avoid repeating TFA process.
  tfa_accept_code($account->uid);
  watchdog('tfa', 'TFA login code accepted for @name', array('@name' => $user->name));
  // Truncate flood for user.
  db_query("DELETE FROM {flood} WHERE event = '%s' AND hostname = '%s'", 'tfa_send', ip_address());
  db_query("DELETE FROM {flood} WHERE event = '%s' AND hostname = '%s'", 'tfa_validate', ip_address());
  user_module_invoke('login', $edit, $user);
}

/**
 * Admin settings form.
 */
function tfa_admin_settings() {
  $form = array();
$account = user_load(array('uid' => 3));
$ready = tfa_ready($account);

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

  // If using the SMS channel a profile field is required.
  if ($default_channel == 'sms') {
    $form['tfa_phone_field'] = array(
      '#value'  => t('<div class="error messages">A phone field is required for the TFA process. Add a profile field for phone number storage to continue. Consult the <a href="!url">help documentation for more info</a>.</div>', array('!url' => url('admin/help/tfa'))),
    );
  }
  if (module_exists('profile')) {
    $options = array();
    $result = db_query("SELECT title, name from {profile_fields}");
    while ($row = db_fetch_array($result)) {
      $options[$row['name']] = $row['title'];
    }
    if (!empty($options)) {
      $form['tfa_phone_field'] = array();
      unset($form['tfa_phone_field']['#value']);
      $form['tfa_phone_field']['#type'] = 'select';
      $form['tfa_phone_field']['#default_value'] = variable_get('tfa_phone_field', '');
      $form['tfa_phone_field']['#options'] = $options;
      $form['tfa_phone_field']['#description'] = t('Choose the field that stores phone numbers.');
    }
  }

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

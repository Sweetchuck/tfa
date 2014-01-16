<?php

/**
 * @file tfa.form.inc
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
  $window = variable_get('tfa_flood_window', 3600);
  if (!variable_get('tfa_test_mode', 0) && !flood_is_allowed('tfa_validate', variable_get('tfa_validate_threshold', 5), $window)) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again in !time minutes.', array('!time' => $window / 60)), 'error');
    return drupal_access_denied();
  }
  // Check entire process flood.
  elseif (!variable_get('tfa_test_mode', 0) && !flood_is_allowed('tfa_begin', variable_get('tfa_begin_threshold', 3), $window)) {
    drupal_set_message(t('You have reached the threshold for TFA attempts. Please try again in !time minutes.', array('!time' => $window / 60)), 'error');
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
  if ($tfa->hasFallback()) {
    $form['fallback'] = array(
      '#type' => 'submit',
      '#value' => t("Can't access your account?"),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );
  }
  // Set account element.
  $form['account'] = array(
    '#type' => 'value',
    '#value' => $account,
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
  $account = $form['account']['#value'];
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
  $account = $form['account']['#value'];
  $tfa = tfa_get_process($account);
  $tfa->submitForm($form_state);

  if (!$tfa->processComplete()) {
    // Store TFA context for form rebuild.
    $form_state['storage']['tfa_context'] = $tfa->getContext();
    $form_state['rebuild'] = TRUE;
    // Clear validation flood.
    flood_clear_event('tfa_validate');
  }
  else {
    // TFA process is complete so finalize and authenticate user.
    $context = tfa_get_context($account);
    $tfa->finalize();
    tfa_login($account);
    // Set redirect based on query parameters, existing $form_state or context.
    $form_state['redirect'] = _tfa_form_get_destination($context, $form_state);
  }
}

/**
 * Get destination and options for $form_state['redirect'].
 *
 * @param array $context TFA account context
 * @param array $form_state Form API state array from tfa_form_submit()
 * @return array
 */
function _tfa_form_get_destination($context, $form_state) {
  $query = drupal_get_query_parameters();
  $options = array();
  $destination = 'user';
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
  if (!empty($form_state['redirect'])) {
    if (is_array($form_state['redirect'])) {
      $destination = $form_state['redirect'][0];
      $options = $form_state['redirect'][1];
    }
    else {
      $destination = $form_state['redirect'];
    }
  }
  if (!empty($query['destination'])) {
    $destination = $query['destination'];
    unset($query['destination']);
    unset($_GET['destination']);
  }
  // Context redirect takes final precedence.
  if (!empty($context['redirect'])) {
    if (is_array($context['redirect'])) {
      $destination = $context['redirect'][0];
      $options = $context['redirect'][1];
    }
    else {
      $destination = $context['redirect'];
    }
  }
  $options = array_merge($options, array('query' => $query));
  return array($destination, $options);
}

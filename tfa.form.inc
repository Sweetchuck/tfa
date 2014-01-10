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
  if (!flood_is_allowed('tfa_validate', variable_get('tfa_validate_threshold', 5))) {
    drupal_set_message(t('You have reached the threshold for incorrect code entry attempts. Please try again later.'), 'error');
    return drupal_access_denied();
  }
  // Check entire process flood.
  elseif (!flood_is_allowed('tfa_begin', variable_get('tfa_begin_threshold', 3))) {
    drupal_set_message(t('You have reached the threshold for TFA attempts. Please try again later.'), 'error');
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
  }
  else {
    // TFA process is complete so finalize and authenticate user.
    $context = tfa_get_context($account);
    $tfa->finalize();
    tfa_login($account);

    // Set redirect based on query parameters, existing $form_state or context.
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
    $form_state['redirect'] = array($destination, $options);
  }
}

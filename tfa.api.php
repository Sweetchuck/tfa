<?php

/**
 * @file
 * This file contains no working PHP code; it exists to provide additional
 * documentation for doxygen as well as to document hooks in the standard
 * Drupal manner.
 */

/**
 * @defgroup tfa TFA module integrations.
 *
 * Module integrations with the TFA module.
 *
 */

/**
 * Define TFA communication channel and address storage.
 *
 * This hook is required to provide a TFA channel for login code communication
 * or to provide custom storage of a address (e.g. phone number) for an account.
 *
 * @return
 *   An array of information about the module's methods for TFA integration.
 *
 *   Required attributes are:
 *
 *    - 'title'
 *        Human-readable title of the communication channel.
 *    - 'send callback'
 *        Function to call for login code transfer. Arguments are $account (a
 *        fully loaded user object being authenticated), $code (the login code,
 *        string), $message (message to accompany code, string). Must return
 *        boolean TRUE or FALSE.
 *    - 'address callback'
 *        Function to call for a unique address for the account being
 *        authenticated. Arguments are $account, a fully loaded user object.
 *        Must return a non-empty variable or FALSE.
 */
function hook_tfa_api() {
  return array(
    'title' => t('TFA Example Channel'),
    'send callback' => '_tfa_send_code',
    'address callback' => '_tfa_lookup_address',
  );
}

/**
 * Example TFA API implementation to use email as code transfer channel.
 */

/**
 * Implements hook_tfa_api().
 */
function email_tfa_tfa_api() {
  return array(
    'title' => t('Email code'),
    'send callback' => 'email_tfa_send',
    'address callback' => 'email_tfa_get_address',
  );
}

/**
 * Address collector for Email TFA.
 */
function email_tfa_get_address($account) {
  return $account->mail;
}

/**
 * Send method for Email TFA.
 */
function email_tfa_send($account, $code, $message) {
  $params = array(
    'code' => $code,
    'message' => $message,
  );
  $from = variable_get('site_mail', 'admin@example.com');
  $result = drupal_mail('email_tfa', 'tfa_send', $account->mail, user_preferred_language($account), $params, $from, TRUE);
  return $result['result'];
}

/**
 * Implements hook_mail().
 */
function email_tfa_mail($key, &$message, $params) {
  if ($key == 'tfa_send') {
    $message['subject'] = t('Login code from @site-name', array('@site-name' => variable_get('site_name', 'Drupal')));
    $message['body'][] = t('@message', array('@message' => $params['message']));
    $message['body'][] = t('@code', array('@code' => $params['code']));
  }
}
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
 */

/**
 * There are two ways to integrate your module with TFA. You can (1) provide
 * methods for communicating TFA codes and storing the address field and (2)
 * alter the functions involved with beginning two-factor authentication for a
 * user.
 *
 * To provide your own code communication channel implement hook_tfa_api() and
 * set attributes as necessary. See the next section for details.
 *
 * To alter or ammend the TFA process implement hook_tfa_processes_alter().
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
    'send callback' => '_tfa_example_send_code',
    'address callback' => '_tfa_example_lookup_address',
  );
}

/**
 * Ammend or alter the TFA authentication process.
 *
 * Implement hook_tfa_processes_alter() to ammend the order and definition of
 * functions to invoke while executing two-factor authentication for an account.
 *
 * Each process defined by a hook_tfa_processes_alter() must be included in the
 * running PHP process. Each function is passed the user object account being
 * executed on and an alterable array of contextual information about the
 * current TFA process. Core TFA will include the 'tfa_code' element in the
 * context to establish where in the TFA process the user resides. A TFA code is
 * an array containing elements string 'code', 'accepted' which evaluates to
 * true or false, and integer 'created'.
 *
 * Note, there is no flood protection when not using TFA's default code process.
 *
 * @see tests/tfa_test.module for more example usuage.
 */
function hook_tfa_processes_alter(&$processes) {
  $processes[] = '_tfa_example_process_callback';
}

/**
 * Example TFA API implementation to use email as communication channel.
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

/**
 * Example TFA process chain alter to skip TFA code verification for users with
 * the administrator role.
 *
 * Note, the following code is redundant with core TFA permissions but is here
 * for illustration.
 */

/**
 * Implements hook_tfa_process_alter to alter the TFA process chain.
 *
 * @param $processes Array of functions in the TFA process chain.
 */
function admin_user_tfa_processes_alter(&$processes) {
  // Push this process callback before default TFA process to ensure it is run
  // before TFA redirects request to code entry form.
  array_unshift($processes, 'admin_user_skip_tfa');
}

/**
 * TFA process callback for admin_user example.
 *
 * @param $account User account object.
 * @param $context Alterable context about the running TFA process.
 *
 * @return string Constants TFA_PROCESS_LOGIN to denote that user may skip any
 * remaining TFA processes and authenticate or TFA_PROCESS_NEXT so next process
 * is invoked.
 */
function admin_user_skip_tfa($account, &$context) {
  if (array_key_exists(3, $account->roles)) {
    return TFA_PROCESS_LOGIN;
  }
  return TFA_PROCESS_NEXT;
}
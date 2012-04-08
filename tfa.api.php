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
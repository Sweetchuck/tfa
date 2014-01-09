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
 * Define TFA plugins.
 *
 * This hook is required to use your own TFA plugin.
 *
 * A plugin must extend the TfaBasePlugin class and may implement one or more of
 * the TFA plugin interfaces.
 *
 * Note, user-defined plugin classes must be available to the Drupal registry
 * for loading.
 *
 * @return
 *   Keyed array of information about the plugin for TFA integration.
 *
 *   Required key:
 *
 *    - 'machine_name'
 *      Unique machine name identifying the plugin.
 *
 *    With required sub-array containing:
 *
 *    - 'name'
 *       Human-readable name of the plugin.
 *    - 'class'
 *       Class name of the plugin.
 */
function hook_tfa_api() {
  return array(
    'my_tfa_plugin' => array(
      'name'  => 'My TFA plugin',
      'class' => 'MyTfaPlugin',
    ),
  );
}


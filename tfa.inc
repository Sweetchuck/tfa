<?php

/**
 * @file TFA module classes
 */

/**
 * Class Tfa
 */
class Tfa {

  public $hasFallback;

  protected $plugins = array();
  protected $plugin_info;
  protected $account;
  protected $context;
  protected $plugin;
  protected $loginPlugin;
  protected $sendPlugin;
  protected $validatePlugin;
  protected $fallbackPlugins;
  protected $complete;

  /**
   * TFA constructor.
   *
   * @param [type] $plugins [description]
   * @param array  $context [description]
   */
  public function __construct($plugins, array $context = array()) {
    $this->validatePlugin = NULL;
    if (!empty($plugins['validate'])) {
      $this->validatePlugin = new $plugins['validate']($context);
    }
    $this->loginPlugin = NULL;
    if (!empty($plugins['login'])) {
      $this->loginPlugin = new $plugins['login']($context);
    }
    $this->hasFallback = FALSE;
    if (!empty($plugins['fallback'])) {
      // @todo may not work since submitForm will need to check ready()
      $this->fallbackPlugins = $plugins['fallback'];
      $this->hasFallback = TRUE;
    }
    //$this->plugin = $plugin;
    $this->context = $context;
    $this->context['plugins'] = $plugins;
    $this->complete = FALSE;

  }

  /**
   * Whether authentication should be interrupted.
   *
   * @return bool
   */
  public function loginAllowed() {
    if ($this->loginPlugin) {
      return $this->loginPlugin->loginAllowed();
    }
  }

  /**
   * Determine if TFA process is ready.
   *
   * @return bool Whether process can begin or not.
   */
  public function ready() {
    return TRUE; // @todo invoke plugin ready
  }

  /**
   * Get TFA process form from plugin.
   *
   * @return array Form API array.
   */
  public function getForm() {
    return $this->validatePlugin->getForm(array());
  }

  /**
   * Validate form.
   *
   * @param array $code
   * @return bool
   */
  public function validateForm(&$form_state) {
    return $this->validatePlugin->validateForm($form_state);
  }

  /**
   * Return process error messages.
   *
   * @return array
   */
  public function getErrorMessages() {
    return $this->validatePlugin->getErrorMessages();
  }

  /**
   * [submitForm description]
   * @param  [type] $form_state [description]
   * @return [type]             [description]
   */
  public function submitForm(&$form_state) {
    // Handle fallback if set.
    if ($this->hasFallback && isset($form_state['values']['fallback']) && $form_state['values']['op'] === $form_state['values']['fallback']) {
      //dpm('issuing fallback');
      // Change context to next fallback.
      // @todo check that fallbacks aren't extinguished.
      $this->context['plugins']['validate'] = array_shift($this->context['plugins']['fallback']);
    }
    elseif (method_exists($this->validatePlugin, 'submitForm')) {
      //dpm('calling plugin submitForm');
      $this->validatePlugin->submitForm($form_state);
    }
    // Check if plugin is complete.
    $this->complete = $this->validatePlugin->complete();
  }

  /**
   * Return TFA context.
   *
   * @return array
   */
  public function getContext() {
    return $this->context;
  }

  /**
   *
   * @return bool [description]
   */
  public function processComplete() {
    return $this->complete;
  }

  /**
   * [begin description]
   * @return [type] [description]
   */
  public function begin() {
    if (method_exists($this->validatePlugin, 'send')) {
      $code = $this->validatePlugin->send();
    }
  }

  /**
   * [valid description]
   * @return [type] [description]
   */
  public function valid() {
    return $this->validatePlugin->valid();
  }

  /**
   * [finalize description]
   * @return [type] [description]
   */
  public function finalize() {
    // Delete code if necessary
    if (method_exists($this->validatePlugin, 'finalize')) {
      $this->validatePlugin->finalize();
    }
  }

}

/**
 * Base plugin class.
 */
abstract class TfaBasePlugin {

  protected $code;
  protected $codeLength;
  protected $context;
  protected $complete;
  protected $valid;
  protected $errorMessages;

  /**
   * Plugin constructor.
   *
   * @param array $context [description]
   */
  public function __construct(array $context = array()) {
    $this->context = $context;
    $this->codeLength = 6; // @todo
    $this->complete = FALSE;
    $this->valid = FALSE;
    $this->errorMessages = array();
  }

  /**
   * Get error messages in format usable by form_set_error().
   *
   * @return array
   */
  public function getErrorMessages() {
    return $this->errorMessages;
  }

  /**
   * Submit form.
   *
   * @param array $form_state
   * @return bool Whether form passes validation or not
   */
  public function submitForm($form_state) {
    $this->complete = TRUE;
  }

  /**
   * Whether the plugin's code processing is valid.
   *
   * @param  [type] $code [description]
   * @return [type]       [description]
   */
  public function valid() {
    return $this->valid;
  }

  /**
   * Whether the plugin's processing is complete.
   *
   * @return [type] [description]
   */
  public function complete() {
    return $this->complete;
  }

  /**
   * Validate code and set internal valid property.
   *
   * @param string $code Code to be validated
   * @return bool Whether code is valid
   */
  protected function validate($code) {
    if ((string) $code === (string) $this->code) {
      $this->valid = TRUE;
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  /**
   * [generate description]
   * @return [type] [description]
   */
  protected function generate() {
    return substr(str_shuffle(str_repeat("123456789abcdefghjkmnpqrstuvwxyz", 5)), 0, $this->codeLength);
  }

  /**
   * [encrypt description]
   * @param  [type] $text [description]
   * @return [type]       [description]
   */
  protected function encrypt($text) {
    $key = $this->key;

    $td = mcrypt_module_open('rijndael-128', '', 'ecb', '');
    $iv = mcrypt_create_iv(mcrypt_enc_get_iv_size($td), MCRYPT_RAND);

    $key = substr($key, 0, mcrypt_enc_get_key_size($td));

    mcrypt_generic_init($td, $key, $iv);

    $data = mcrypt_generic($td, $text);

    mcrypt_generic_deinit($td);
    mcrypt_module_close($td);

    return $data;
  }

  /**
   * [decrypt description]
   * @param  [type] $data [description]
   * @return [type]       [description]
   */
  protected function decrypt($data) {
    $key = $this->key;

    $td = mcrypt_module_open('rijndael-128', '', 'ecb', '');
    $iv = mcrypt_create_iv(mcrypt_enc_get_iv_size($td), MCRYPT_RAND);

    $key = substr($key, 0, mcrypt_enc_get_key_size($td));

    mcrypt_generic_init($td, $key, $iv);

    $text = mdecrypt_generic($td, $data);

    mcrypt_generic_deinit($td);
    mcrypt_module_close($td);

    return $text;
  }
}

/**
 * Interface TfaValidationPlugin
 *
 * Validation plugins interact with the TFA form processes to provide code entry
 * and validate submitted codes.
 */
interface TfaValidationPlugin {

  /**
   * Get TFA process form from plugin.
   *
   * @param array Existing form.
   * @return array Form API array.
   */
  public function getForm($form);

  /**
   * Validate form.
   *
   * @param array $form_state
   * @return bool Whether form passes validation or not
   */
  public function validateForm(&$form_state);
}

/**
 * Interface TfaLoginPlugin
 *
 * Login plugins interact with the TFA loginAllowed() process prior to starting
 * a TFA process.
 */
interface TfaLoginPlugin {

  /**
   * Whether authentication should be interrupted.
   *
   * @return bool
   */
  public function loginAllowed();
}

/**
 * Interface TfaSendPlugin
 *
 * Send plugins interact with the TFA begin() process to communicate a code
 * during the start of the TFA process.
 *
 * Implementations of a send plugin should also be a validation plugin.
 */
interface TfaSendPlugin {

  /**
   * Transfer code.
   */
  public function send();
}

<?php

/**
 * @file TFA module classes
 */

/**
 * Class Tfa
 */
class Tfa {

  protected $plugins = array();
  protected $plugin_info;
  protected $account;
  protected $context;
  protected $validatePlugin = NULL;
  protected $loginPlugins = array();
  protected $fallbackPlugins = array();
  protected $complete = FALSE;
  protected $fallback = FALSE;

  /**
   * TFA constructor.
   *
   * @param array $plugins
   *   Plugins to instansiate.
   *
   *   Must include key:
   *
   *     - 'validate'
   *       Class name of TfaPlugin implementing TfaValidationPlugin.
   *
   *   May include keys:
   *
   *     - 'login'
   *       Class name of TfaPlugin implementing TfaLoginPlugin.
   *
   *     - 'fallback'
   *       Array of classes of TfaPlugin that can be used as fallback processes.
   *
   * @param array $context
   *   Context of TFA process.
   *
   *   Must include key:
   *
   *     - 'uid'
   *       Account uid of user in TFA process.
   *
   *   May include key:
   *
   *     - 'redirect'
   *       String or array suitable for $form_state['redirect'].
   */
  public function __construct(array $plugins, array $context) {
    if (!empty($plugins['validate'])) {
      $this->validatePlugin = new $plugins['validate']($context);
    }
    if (!empty($plugins['login'])) {
      foreach ($plugins['login'] as $class) {
        $this->loginPlugins[] = new $class($context);
      }
    }
    if (!empty($plugins['fallback'])) {
      // @todo may not work since submitForm will need to check ready()
      $this->fallbackPlugins = $plugins['fallback'];
      $this->fallback = TRUE;
    }
    $this->context = $context;
    $this->context['plugins'] = $plugins;
  }

  /**
   * Whether authentication should be allowed and not interrupted.
   *
   * If any plugin returns TRUE then authentication is not interrupted by TFA.
   *
   * @return bool
   */
  public function loginAllowed() {
    if (!empty($this->loginPlugins)) {
      foreach ($this->loginPlugins as $class) {
        if ($class->loginAllowed()) {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

  /**
   * Determine if TFA process is ready.
   *
   * @return bool Whether process can begin or not.
   */
  public function ready() {
    return $this->validatePlugin->ready();
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
    if ($this->fallback && isset($form_state['values']['fallback']) && $form_state['values']['op'] === $form_state['values']['fallback']) {
      // Change context to next fallback.
      $this->context['plugins']['validate'] = array_shift($this->context['plugins']['fallback']);
      if (empty($this->context['plugins']['fallback'])) {
        $this->fallback = FALSE;
      }
    }
    // Otherwise invoke plugin submitForm().
    elseif (method_exists($this->validatePlugin, 'submitForm')) {
      $this->validatePlugin->submitForm($form_state);
    }
    // Check if plugin is complete.
    $this->complete = $this->validatePlugin->complete();
  }

  /**
   * Begin the TFA process.
   */
  public function begin() {
    // Invoke send method on send validation plugins.
    if (method_exists($this->validatePlugin, 'send')) {
      $code = $this->validatePlugin->send();
    }
  }

  /**
   * Whether the TFA process has any fallback proceses.
   *
   * @return bool
   */
  public function hasFallback() {
    return $this->fallback;
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
   * Whether the TFA process is complete.
   *
   * @return bool
   */
  public function processComplete() {
    return $this->complete;
  }

  /**
   * Whether the TFA process is valid.
   *
   * @return bool
   */
  public function valid() {
    return $this->validatePlugin->valid();
  }

  /**
   * Run TFA process finalization.
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
  protected $complete = FALSE;
  protected $valid = FALSE;
  protected $errorMessages = array();
  protected $encryptionKey;

  /**
   * Plugin constructor.
   *
   * @param array $context
   *   Context of current TFA process.
   *
   *   Must include key:
   *
   *     - 'uid'
   *       Account uid of user in TFA process.
   */
  public function __construct(array $context = array()) {
    $this->context = $context;
    $this->codeLength = 6; // @todo
  }

  /**
   * Determine if the plugin can run for the current TFA context.
   *
   * @return bool
   */
  public function ready() {
    return TRUE;
  }

  /**
   * Get error messages suitable for form_set_error().
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
   * @return bool
   */
  public function valid() {
    return $this->valid;
  }

  /**
   * Whether the plugin's processing is complete.
   *
   * @return bool
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
    }
    else {
      $this->valid = FALSE;
    }
    return $this->valid;
  }

  /**
   * Generate a random string of characters of length $this->codeLength.
   *
   * @return string
   */
  protected function generate() {
    $code = substr(str_shuffle(str_repeat("123456789abcdefghjkmnpqrstuvwxyz", 5)), 0, $this->codeLength);
    $this->code = $code;
    return $code;
  }

  /**
   * Encrypt a plaintext string.
   *
   * Should be used when writing codes to storage.
   *
   * @param string.
   * @return string
   */
  protected function encrypt($text) {
    $key = $this->encryptionKey;

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
   * Decrypt a encrypted string.
   *
   * Should be used when reading codes from storage.
   *
   * @param string
   * @return string
   */
  protected function decrypt($data) {
    $key = $this->encryptionKey;

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
interface TfaValidationPluginInterface {

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
interface TfaLoginPluginInterface {

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
interface TfaSendPluginInterface {

  /**
   * Transfer code.
   */
  public function send();
}

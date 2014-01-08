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
   * [__construct description]
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
   *
   * @return bool [description]
   */
  public function loginAllowed() {
    if ($this->loginPlugin) {
      return $this->loginPlugin->loginAllowed();
    }
  }

  /**
   *
   * @return bool [description]
   */
  public function ready() {
    return TRUE; // @todo
  }

  /**
   * [getForm description]
   * @return [type] [description]
   */
  public function getForm() {
    //dpm(get_class($this->validatePlugin), 'getForm');
    return $this->validatePlugin->getForm(array());
  }

  /**
   * [submitForm description]
   * @param  [type] $form_state [description]
   * @return [type]             [description]
   */
  public function submitForm($form_state) {
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
    // Invoke
    if (method_exists($this->validatePlugin, 'begin')) {
      $code = $this->validatePlugin->begin();
    }
  }

  /**
   * [validate description]
   * @param  [type] $code [description]
   * @return [type]       [description]
   */
  public function validate($code) {
    // Invoke validate method on plugins
    return $this->validatePlugin->validate($code);
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
 *
 */
abstract class TfaPlugin {

  protected $code;
  protected $codeLength;
  protected $context;
  protected $complete;

  /**
   * [__construct description]
   * @param array $context [description]
   */
  public function __construct(array $context = array()) {
    $this->context = $context;
    $this->complete = FALSE;
    $this->codeLength = 6; // @todo
  }

  /**
   * [validate description]
   * @param  [type] $code [description]
   * @return [type]       [description]
   */
  public function validate($code) {
    return (string) $code === (string) $this->code;
  }

  /**
   * [submitForm description]
   * @param  [type] $form_state [description]
   * @return [type]             [description]
   */
  public function submitForm($form_state) {
    $this->complete = TRUE;
  }

  /**
   * [complete description]
   * @return [type] [description]
   */
  public function complete() {
    return $this->complete;
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
 *
 */
interface TfaLoginPlugin {

  /**
   * [loginAllowed description]
   * @return [type] [description]
   */
  public function loginAllowed();
}

/**
 *
 */
interface TfaTotpPlugin {

  /**
   * [getForm description]
   * @param  [type] $form [description]
   * @return [type]       [description]
   */
  public function getForm($form);

  /**
   * [validate description]
   * @param  [type] $code [description]
   * @return [type]       [description]
   */
  public function validate($code);
}

/**
 *
 */
interface TfaSendPlugin {

  /**
   * [begin description]
   * @return [type] [description]
   */
  public function begin();

  /**
   * [getForm description]
   * @param  [type] $form [description]
   * @return [type]       [description]
   */
  public function getForm($form);

  /**
   * [validate description]
   * @param  [type] $code [description]
   * @return [type]       [description]
   */
  public function validate($code);
}

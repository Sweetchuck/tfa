<?php

class Tfa {

  protected $plugins = array();
  protected $plugin_info;
  protected $account;
  protected $context;
  protected $plugin;
  protected $loginPlugin;
  protected $sendPlugin;
  protected $validatePlugin;

  public function __construct($plugins, array $context = array()) {
    //$this->instantiatePlugins();
    //$this->sendPlugin = isset($plugins['send']) ? $plugins['send'] : NULL;
    $this->validatePlugin = NULL;
    if (isset($plugins['validate'])) {
      $this->validatePlugin = new $plugins['validate']($context);
    }
    $this->loginPlugin = NULL;
    if (isset($plugins['login'])) {
      $this->loginPlugin = new $plugins['login']($context);
    }
    //$this->plugin = $plugin;
    $this->context = $context;
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

  public function begin() {
    // Invoke
    if (method_exists($this->validatePlugin, 'begin')) {
      $code = $this->validatePlugin->begin();
    }
  }

  public function validate($code) {
    // Invoke validate method on plugins
    return $this->validatePlugin->validate($code);
  }

  public function finalize() {
    // Delete code if necessary
    if (method_exists($this->validatePlugin, 'finalize')) {
      $this->validatePlugin->finalize();
    }
  }

}

abstract class TfaPlugin {

  protected $code;
  protected $codeLength;
  protected $context;

  public function __construct(array $context = array()) {
    $this->context = $context;
  }

  public function validate($code) {
    return $code === $this->code;
  }

  protected function generate() {
    $codeLength = 6; // @todo
    return substr(str_shuffle(str_repeat("123456789abcdefghjkmnpqrstuvwxyz", 5)), 0, $codeLength);
  }

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

interface TfaLoginPlugin {
  public function loginAllowed();
}

interface TfaTotpPlugin {
  public function validate($code);
}

interface TfaSendPlugin {
  public function begin();
}

<?php

class Tfa {

  protected $plugins = array();
  protected $plugin_info;

  public function __construct() {
    //
  }

  /**
   *
   * @return bool [description]
   */
  public function loginAllowed() {
  }

  /**
   *
   * @return bool [description]
   */
  public function tfaReady() {
  }

  public function beginProcess() {
    // stores context
  }

  /**
   *
   * @return array [description]
   */
  public function entryForm() {

  }

  public function validateForm() {
    // sets validity
  }

  public function submitForm() {
    // cleanup
    // sets redirect?
  }

  /**
   *
   * @return bool [description]
   */
  public function validEntry() {
  }

  /**
   * Create plugin instances for all plugins and add to the data.
   * @return array
   */
  protected function instantiatePlugins() {
    $plugins = $this->getPluginInfo();
    foreach ($plugins as $plugin_name => &$plugin_info) {
      $plugin_info['plugin'] = new $plugin_info['class']();
    }

    return $plugins;
  }

  /**
   * Searches available classes for ones implementing the
   * UserAuthValidatePlugin interface and stores them as a ReflectionClass.
   */
  protected function getPluginInfo() {
    // Return plugins array if they have already been collected.
    if (isset($this->plugin_info)) {
      return $this->plugin_info;
    }

    // If it's cached, get that and use it.
    if ($cache = cache_get('tfa_api:plugin_info')) {
      $this->plugin_info = $cache->data;
    }
    // Otherwise, rebuild data.
    else {
      $plugins = array();
      foreach (module_invoke_all('tfa_api') as $key => $class) {
        // Invoke reflection class to do plugin search.
        $reflection = new ReflectionClass($class);
        if ($reflection->implementsInterface('TfaApiPlugin')) {
          $plugin = $reflection->newInstance();
          $plugins[$key] = array(
            'class' => $class,
            'weight' => $plugin->defaultWeight(),
          );
        }
      }

      // Alter.
      drupal_alter('tfa_api', $plugins);

      $this->plugin_info = $plugins;
      // Set this in the cache.
      cache_set('tfa_api:plugin_info', $plugins);
    }

    return $this->plugin_info;
  }

}

/**
 * Interface for defining TFA API plugins.
 */
interface TfaApiPlugin {

  public function loginAllowed();

  /**
   * Main method to decide validation result.
   */
  public function validate();

  /**
   * This method will be called if validation fails.
   */
  public function validateError();

  /**
   * This method will be called if validation succeeds.
   */
  public function validateSuccess();

  /**
   * The default weight of the plugin. This affects the order it is called
   * if there are multiple plugins.
   */
  public function defaultWeight();

}

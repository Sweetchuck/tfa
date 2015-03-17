<?php

/**
 * @file
 * tfa.admin.inc
 */

/**
 * Admin settings form.
 */
function tfa_admin_settings(&$form_state) {
  // Check if mcrypt plugin is available.
  if (!extension_loaded('mcrypt')) {
    // @todo Allow alter in case of other encryption libs.
    drupal_set_message(t('The TFA module requires the PHP Mcrypt extension be installed on the web server. See <a href="!link">the TFA help documentation</a> for setup.', array('!link' => url('admin/help/tfa'))), 'error');

    return array();
  }

  // Gather plugins.
  $plugins = $send_plugins = $validate_plugins = $login_plugins = array();
  foreach (module_invoke_all('tfa_api') as $plugin_name => $plugin) {
    // @todo We are using interfaces. Why we enforce the plugins to extend the
    // TfaBasePlugin base class?
    if (is_subclass_of($plugin['class'], 'TfaBasePlugin')) {
      $plugins[$plugin_name] = $plugin;
    }

    if (in_array('TfaValidationPluginInterface', class_implements($plugin['class']))) {
      $validate_plugins[$plugin_name] = $plugin['name'];
    }

    if (in_array('TfaSendPluginInterface', class_implements($plugin['class']))) {
      $send_plugins[$plugin_name] = $plugin['name'];
    }
    elseif (in_array('TfaLoginPluginInterface', class_implements($plugin['class']))) {
      $login_plugins[$plugin_name] = $plugin['name'];
    }
  }

  // Return if there are no plugins.
  if (empty($plugins) || empty($validate_plugins)) {
    drupal_set_message(t('No plugins available for validation. See <a href="!link">the TFA help documentation</a> for setup.', array('!link' => url('admin/help/tfa'))), 'error');

    return array();
  }

  if (isset($form['#attributes']['class'])) {
    $form['#attributes']['class'] .= ' tfa-admin-settings-form';
  }
  else {
    $form['#attributes']['class'] = 'tfa-admin-settings-form';
  }

  $path = drupal_get_path('module', 'tfa');
  drupal_add_js("$path/js/tfa.admin_settings.js");

  $enabled = variable_get('tfa_enabled', FALSE);

  $form['plugins'] = array(
    '#type' => 'fieldset',
    '#title' => t('Available plugins'),
  );

  $items = array();
  foreach ($plugins as $plugin_name => $plugin) {
    $args = array(
      '%type' => implode(', ', _tfa_class_types($plugin['class'])),
      '@name' => $plugin['name'],
    );

    // Include message whether plugin is set.
    if ($enabled && variable_get('tfa_validate_plugin', '') === $plugin_name) {
      $items[] = t('<strong>@name</strong> (%type) - active validator', $args);
    }
    elseif ($enabled && in_array($plugin_name, variable_get('tfa_login_plugins', array()))) {
      $items[] = t('<strong>@name</strong> (%type) - active login', $args);
    }
    elseif ($enabled && in_array($plugin_name, variable_get('tfa_fallback_plugins', array()))) {
      $items[] = t('<strong>@name</strong> (%type) - active fallback', $args);
    }
    elseif ($enabled) {
      $items[] = t('<strong>@name</strong> (%type) - unused', $args);
    }
    else {
      $items[] = t('<strong>@name</strong> (%type)', $args);
    }
  }

  $form['plugins']['list'] = array(
    '#type' => 'markup',
    '#value' => theme('item_list', $items),
  );

  // Option to enable entire process or not.
  $form['tfa_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable TFA'),
    '#default_value' => $enabled,
    '#description' => t('Enable TFA for account authentication.'),
  );

  $form['tfa_options'] = array(
    '#prefix' => '<div class="tfa-options-wrapper">',
    '#suffix' => '</div>',
  );

  // Default active plugin.
  if (count($validate_plugins) >= 1) {
    $form['tfa_options']['tfa_validate'] = array(
      '#type' => 'select',
      '#title' => t('Default validation plugin'),
      '#options' => $validate_plugins,
      '#default_value' => variable_get('tfa_validate_plugin', ''),
      '#description' => t('Plugin that will be used as the default TFA process.'),
    );
  }
  else {
    $form['tfa_options']['no_validate'] = array(
      '#type' => 'markup',
      '#value' => t('No available validation plugins available. TFA process will not occur.'),
    );
  }

  // Order of fallback plugins.
  if (count($validate_plugins) > 1) {
    $enabled_fallback = array_values(variable_get('tfa_fallback_plugins', array()));
    $form['tfa_options']['tfa_fallback'] = array(
      '#type' => 'fieldset',
      '#title' => t('Validation fallback plugins'),
      '#description' => t('Fallback plugins and order. Note, if a fallback plugin is not setup for an account it will not be active in the TFA form.'),
      '#tree' => TRUE,
    );

    // First enabled.
    $delta = count($enabled_fallback) + count($validate_plugins);
    foreach ($enabled_fallback as $weight => $plugin_name) {
      if (!isset($validate_plugins[$plugin_name])) {
        continue;
      }

      $form['tfa_options']['tfa_fallback'][$plugin_name] = array(
        '#weight' => $weight,
        '#prefix' => "<div class=\"tfa-options-fallback-plugin tfa-options-fallback-plugin-{$plugin_name}\">",
        '#suffix' => '</div>',
        'enable' => array(
          '#title' => $validate_plugins[$plugin_name],
          '#type' => 'checkbox',
          '#default_value' => TRUE,
          '#attributes' => array(
            'class' => 'tfa-fallback-plugin-enabler',
          ),
        ),
        'weight' => array(
          '#type' => 'weight',
          '#title' => t('Weight'),
          '#default_value' => $weight,
          '#delta' => $delta,
        ),
      );
    }

    // Then other plugins.
    foreach ($validate_plugins as $plugin_name => $plugin_label) {
      if (isset($form['tfa_options']['tfa_fallback'][$plugin_name])) {
        continue;
      }

      $weight = (int) array_search($plugin_name, $enabled_fallback);
      $form['tfa_options']['tfa_fallback'][$plugin_name] = array(
        '#prefix' => "<div class=\"tfa-options-fallback-plugin tfa-options-fallback-plugin-{$plugin_name}\">",
        '#suffix' => '</div>',
        '#weight' => $weight,
        'enable' => array(
          '#title' => $plugin_label,
          '#type' => 'checkbox',
          '#default_value' => in_array($plugin_name, $enabled_fallback),
          '#attributes' => array(
            'class' => 'tfa-fallback-plugin-enabler',
          ),
        ),
        'weight' => array(
          '#type' => 'weight',
          '#title' => t('Weight'),
          '#default_value' => $weight,
          '#delta' => $delta,
        ),
      );
    }
  }

  // Enable login plugins.
  if (count($login_plugins) >= 1) {
    $form['tfa_options']['tfa_login'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Login plugins'),
      '#options' => $login_plugins,
      '#default_value' => variable_get('tfa_login_plugins', array()),
      '#description' => t('Plugins that can allow a user to skip the TFA process. If any plugin returns true the user will not be required to follow TFA. <strong>Use with caution.</strong>'),
    );
  }

  $form['actions']['#weight'] = 500;
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

/**
 * Admin form submission handler.
 */
function tfa_admin_settings_submit($form, &$form_state) {
  drupal_set_message(t('Configuration saved'));
  $values = $form_state['values'];
  // Set enabled.
  if (!empty($values['tfa_enabled'])) {
    variable_set('tfa_enabled', $values['tfa_enabled']);
  }
  else {
    variable_del('tfa_enabled');

    return;
  }

  // Set main validation plugins.
  variable_set('tfa_validate_plugin', $values['tfa_validate']);

  // Set fallback plugins.
  $fallback = array();
  if (!empty($values['tfa_fallback'])) {
    foreach ($values['tfa_fallback'] as $key => $data) {
      if ($values['tfa_validate'] !== $key && $data['enable']) {
        $fallback[$data['weight']] = $key;
      }
    }
  }
  // Always include the default validation plugin in case the context plugins
  // are altered. TFA will skip it if it's the same as validation at runtime.
  $fallback[-999] = $values['tfa_validate'];
  ksort($fallback);
  variable_set('tfa_fallback_plugins', $fallback);

  // Set login plugins.
  $login = array();
  if (!empty($values['tfa_login'])) {
    foreach ($values['tfa_login'] as $key => $enabled) {
      if ($enabled) {
        $login[] = $key;
      }
    }
  }
  variable_set('tfa_login_plugins', $login);
}

/**
 * Get human-readable types.
 *
 * @param string $class
 *   PHP class name.
 *
 * @return string[]
 *   Implemented interface types.
 */
function _tfa_class_types($class) {
  $types = array();

  if (in_array('TfaValidationPluginInterface', class_implements($class))) {
    $types['validation'] = t('Validation');
  }

  if (in_array('TfaSendPluginInterface', class_implements($class))) {
    $types['send'] = t('Send');
  }

  if (in_array('TfaLoginPluginInterface', class_implements($class))) {
    $types['login'] = t('Login');
  }

  if (empty($types)) {
    $types['standard'] = t('standard');
  }

  return $types;
}

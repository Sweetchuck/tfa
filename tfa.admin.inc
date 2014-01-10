<?php

/**
 * @file tfa.admin.inc
 */

/**
 * Admin settings form.
 */
function tfa_admin_settings($form, $form_state) {
  // Gather plugins.
  $plugins = $send_plugins = $validate_plugins = $login_plugins = array();
  foreach (module_invoke_all('tfa_api') as $key => $data) {
    if (is_subclass_of($data['class'], 'TfaBasePlugin')) {
      $plugins[$key] = $data;
    }
    if (in_array('TfaValidationPlugin', class_implements($data['class']))) {
      $validate_plugins[$key] = $data['name'];
    }
    if (in_array('TfaSendPlugin', class_implements($data['class']))) {
      $send_plugins[$key] = $data['name'];
    }
    elseif (in_array('TfaLoginPlugin', class_implements($data['class']))) {
      $login_plugins[$key] = $data['name'];
    }

  }

  // Return if there are no plugins.
  if (empty($plugins) || empty($validate_plugins)) {
    drupal_set_message(t('No plugins available for validation. See <a href="!link">the TFA help documentation</a> for setup.', array('!link' => url('admin/help/tfa'))), 'error');
    return array();
  }

  // Option to enable entire process or not.
  $enabled = variable_get('tfa_enabled', 0);
  $form['tfa_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable TFA'),
    '#default_value' => $enabled,
    '#description' => t('Enable TFA for account authentication.'),
  );

  // Force set test plugins. @todo
  //variable_set('tfa_login_plugin', 'tfa_deviceauth');
  //variable_set('tfa_validate_plugin', 'tfa_totp');
  //variable_set('tfa_fallback_plugins', array('tfa_email', 'tfa_recovercode'));

  $form['plugins'] = array(
    '#type' => 'fieldset',
    '#title' => t('Plugins'),
  );
  $items = array();
  foreach ($plugins as $key => $plugin) {
    $message = '<strong>@name</strong> (%type)';
    // Include message whether plugin is set.
    if ($enabled && variable_get('tfa_validate_plugin', '') === $key) {
      $message .= ' - active validator';
    }
    elseif ($enabled && variable_get('tfa_login_plugin', '') === $key) {
      $message .= ' - active login';
    }
    elseif ($enabled && in_array($key, variable_get('tfa_fallback_plugins', array()))) {
      $message .= ' - active fallback';
    }
    elseif ($enabled) {
      $message .= ' - unused';
    }
    $items[] = t($message, array('%type' => _tfa_class_types($plugin['class']), '@name' => $plugin['name']));
  }
  $form['plugins']['list'] = array(
    '#value' => 'markup',
    '#markup' => theme('item_list', array('items' => $items)),
  );

  // Default active plugin
  if (count($validate_plugins) >= 1) {
    $form['tfa_validate'] = array(
      '#type' => 'select',
      '#title' => t('Default validation plugin'),
      '#options' => $validate_plugins,
      '#default_value' => variable_get('tfa_validate_plugin', ''),
      '#description' => t('Plugin that will be used as the default TFA process.'),
      '#states' => array(
        'visible' => array(
          ':input[name="tfa_enabled"]' => array('checked' => TRUE),
        )
      ),
    );
  }
  else {
    $form['no_validate'] = array(
      '#value' => 'markup',
      '#markup' => t('No available validation plugins available. TFA process will not occur.'),
    );
  }

  // Order of fallback plugins
  if (count($validate_plugins) > 1) {
    $form['tfa_fallback'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Validation fallback plugins'),
      '#options' => $validate_plugins,
      '#description' => t('Plugins to be set as fallback to default.'),
      '#default_value' => variable_get('tfa_fallback_plugins', array()),
      '#states' => array(
        'visible' => array(
          ':input[name="tfa_enabled"]' => array('checked' => TRUE),
        )
      ),
    );
  }

  // Enable login plugins. @todo make multi-select
  if (count($login_plugins) >= 1) {
    $form['tfa_login'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Login plugins'),
      '#options' => $login_plugins,
      '#default_value' => array(variable_get('tfa_login_plugin', '')),
      '#description' => t('Plugins that can allow a user to skip the TFA process.'),
      '#states' => array(
        'visible' => array(
          ':input[name="tfa_enabled"]' => array('checked' => TRUE),
        )
      ),
    );
  }

  // Allow users to choose their validation method. @todo

  $form['tfa_required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Require TFA'),
    '#default_value' => variable_get('tfa_required', 0),
    '#description' => t('Require TFA process for account login. <strong>Note, any account that is not setup for TFA (and does not have the "skip TFA" permission) will not be able to authenticate if this is set.</strong>'),
    '#states' => array(
      'visible' => array(
        ':input[name="tfa_enabled"]' => array('checked' => TRUE),
      )
    ),
  );

  $form['actions']['#type'] = 'actions';
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
  $values = $form_state['values'];
  dpm($values);
  // Set enabled.
  if (!empty($values['tfa_enabled'])) {
    variable_set('tfa_enabled', $values['tfa_enabled']);
  }
  else {
    variable_del('tfa_enabled');
    return;
  }
  // Set tfa_validate_plugin
  if (!empty($values['tfa_validate'])) {
    variable_set('tfa_validate_plugin', $values['tfa_validate']);
  }
  else {
    variable_del('tfa_validate_plugin');
  }
  // Set tfa_fallback_plugins
  $fallback = array_filter($values['tfa_fallback']);
  if (!empty($fallback)) {
    variable_set('tfa_fallback_plugins', array_keys($fallback));
  }
  else {
    variable_del('tfa_fallback_plugins');
  }
  // Set tfa_login_plugin.
  // @todo support multi-select.
  $login = array_pop($values['tfa_login']);
  if (!empty($login)) {
    variable_set('tfa_login_plugin', $login);
  }
  else {
    variable_del('tfa_login_plugin');
  }
  // Set required.
  variable_set('tfa_required', $values['tfa_required']);
}

function _tfa_class_types($class) {
  $types = array();
  if (in_array('TfaValidationPlugin', class_implements($class))) {
    $types[] = 'validation';
  }
  if (in_array('TfaSendPlugin', class_implements($class))) {
    $types[] = 'send';
  }
  if (in_array('TfaLoginPlugin', class_implements($class))) {
    $types[] = 'login';
  }
  if (empty($types)) {
    $types[] = 'standard';
  }
  return implode(', ', $types);
}

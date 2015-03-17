<?php
/**
 * @file
 * Send plugin tester.
 */

/**
 * Class TfaTestSend.
 */
class TfaTestSend extends TfaBasePlugin implements TfaValidationPluginInterface, TfaSendPluginInterface {

  /**
   * {@inheritdoc}
   */
  public function __construct(array $context = array()) {
    parent::__construct($context);
    $this->code = variable_get('tfa_test_code', 'TEST');
  }

  /**
   * {@inheritdoc}
   */
  public function ready() {
    return variable_get('tfa_test_is_ready', TRUE);
  }

  /**
   * {@inheritdoc}
   */
  public function getForm(array $form, array &$form_state) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Enter sent code'),
    );

    // @todo Do somethings with 'actions'.
    $form['actions']['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );

    $form['actions']['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend'),
      '#submit' => array('tfa_form_submit'),
    );

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array $form, array &$form_state) {
    $op = $form_state['values']['op'];

    if ($op == $form['actions']['login']['#value']) {
      if (!trim($form_state['values']['code'])) {
        $this->errorMessages['code'] = t('!name field is required.', array('!name' => $form['code']['#title']));

        return FALSE;
      }

      if (!parent::validate($form_state['values']['code'])) {
        $this->errorMessages['code'] = t('Invalid sent code');

        return FALSE;
      }
    }
    elseif ($op == $form['actions']['resend']['#value']) {
      // There is nothing to validate.
    }

    return TRUE;
  }

  /**
   * Check resend flood.
   *
   * @param int $window
   *
   * @return bool
   */
  public function floodIsAllowed($window) {
    if (!tfa_flood_is_allowed('tfa_test_resend', variable_get('tfa_test_resend_threshold', 6), $window, $this->context['uid'])) {
      $this->errorMessages['code'] = t('Resend flood hit');

      return FALSE;
    }

    return TRUE;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array $form, array &$form_state) {
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      // Register resend event for flood protection.
      tfa_flood_register_event('tfa_test_resend', 3600, $this->context['uid']);

      // Support causing send error.
      if (!variable_get('tfa_test_send_begin', TRUE)) {
        drupal_set_message('Error during resend', 'error');
      }
      else {
        drupal_set_message('Code resent');
      }

      return FALSE;
    }
    else {
      return parent::submitForm($form, $form_state);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function begin() {
    // Clear resend flood.
    tfa_flood_clear_event('tfa_test_resend');
    // Variable used by TfaTestCase::testSendError().
    if (!variable_get('tfa_test_send_begin', TRUE)) {
      drupal_set_message('Error during send', 'error');
    }
    // A real plugin might send the code to the user.
  }
}

/**
 * Class TfaTestSendSetup.
 */
class TfaTestSendSetup extends TfaTestSend implements TfaSetupPluginInterface {

  /**
   * {@inheritdoc}
   */
  public function getSetupForm(array $form, array &$form_state) {
    if (empty($form_state['storage']['location'])) {
      $form['location'] = array(
        '#type' => 'textfield',
        '#title' => t('Location to send code'),
        '#required' => TRUE,
      );
      $form['actions']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit'),
      );
    }
    else {
      // Include code entry form.
      $form = $this->getForm($form, $form_state);
    }

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateSetupForm(array $form, array &$form_state) {
    if (!empty($form_state['values']['code']) && !$this->validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid code. Please try again.');

      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitSetupForm(array $form, array &$form_state) {
    if (!empty($form_state['values']['location'])) {
      // Code would be sent to $form_state['values']['location']
      $form_state['storage']['location'] = $form_state['values']['location'];

      return FALSE;
    }
    else {
      // Replace test code.
      variable_set('tfa_test_code', $form_state['storage']['location']);

      // Location would be saved.
      return TRUE;
    }

  }
}

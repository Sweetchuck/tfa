<?php

class TfaTestSend extends TfaBasePlugin implements TfaValidationPluginInterface, TfaSendPluginInterface {

  public function __construct(array $context = array()) {
    parent::__construct($context);
    $this->code = variable_get('tfa_test_code', 'TEST');
  }

  public function ready() {
    return variable_get('tfa_test_send_ready', TRUE);
  }

  public function getForm($form) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Enter sent code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
    $form['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend'),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );

    return $form;
  }

  public function validateForm(&$form_state) {
    if (!parent::validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid sent code');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  public function submitForm($form_state) {
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      drupal_set_message('Code resent');
    }
    else {
      parent::submitForm($form_state);
    }
  }

  public function send() {
    // Do nothing.
  }
}

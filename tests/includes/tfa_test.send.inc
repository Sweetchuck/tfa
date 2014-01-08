<?php

class TfaTestSend extends TfaPlugin implements TfaSendPlugin {

  public function getForm($form) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Enter sent code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );
    $form['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend'),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );

    return $form;
  }

  public function submitForm($form_state) {
    // Resend code if pushed.
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      drupal_set_message('Code resent');
      $code = $this->generate();
      $this->send($code);
    }
    else {
      parent::submitForm($form_state);
    }
  }

  public function begin() {
    $code = $this->generate();
    $this->send($code);
  }

  public function validate($code) {
    $this->code = variable_get('tfa_test', '3lk2jf3');
    return parent::validate($code);
  }

  protected function generate() {
    $code = parent::generate();
    // store code
    //variable_set('tfa_test', $code);
    $code = variable_get('tfa_test', '3lk2jf3');
    return $code;
  }

  protected function send($code) {
    // Do nothing.
  }
}

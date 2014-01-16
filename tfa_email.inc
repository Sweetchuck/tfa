<?php

class TfaEmail extends TfaBasePlugin implements TfaValidationPluginInterface, TfaSendPluginInterface {

  public function getForm($form) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Verification Code'),
      '#required' => TRUE,
      '#description' => t('Enter @length-character code sent to your device.', array('@length' => $this->codeLength)),
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

  public function send() {
    $code = $this->generate();
    $this->sendCode($code);
  }

  public function validateForm(&$form_state) {
    $this->code = variable_get('tfa_test', 'code');
    if (!parent::validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid code.');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  protected function generate() {
    $code = parent::generate();
    // store code
    //variable_set('tfa_test', $code);
    $code = variable_get('tfa_test', '3lk2jf3');
    return $code;
  }

  protected function sendCode($code) {
    /*$params = array(
      'code' => $code,
      'message' => $message,
    );
    $from = variable_get('site_mail', 'admin@example.com');*/
    //$result = drupal_mail('email_tfa', 'tfa_send', $account->mail, user_preferred_language($account), $params, $from, TRUE);
    watchdog('tst', 'tfa send: ' . $code);
  }
}

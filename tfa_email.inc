<?php

class TfaEmail extends TfaPlugin implements TfaSendPlugin {

  public function getForm($form) {
    $form['desc'] = array(
      '#markup' => t('Enter code to continue.'),
    );

    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
    $form['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend code'),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );

    return $form;
  }

  public function submitForm($form_state) {
    // Resend code if pushed.
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
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
    /*$params = array(
      'code' => $code,
      'message' => $message,
    );
    $from = variable_get('site_mail', 'admin@example.com');*/
    //$result = drupal_mail('email_tfa', 'tfa_send', $account->mail, user_preferred_language($account), $params, $from, TRUE);
    watchdog('tst', 'tfa send: ' . $code);
  }
}

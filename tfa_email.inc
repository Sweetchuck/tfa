<?php

class TfaEmail extends TfaPlugin implements TfaSendPlugin {

  public function begin() {
    $code = $this->generate();
    $this->send($code);
  }

  public function validate($code) {
    $this->code = variable_get('tfa_test', '3lk2jf3');
    watchdog('tst', 'email set code ' . $this->code);
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
    watchdog('tst', 'tfa send: ' . var_export($code,1));
  }
}

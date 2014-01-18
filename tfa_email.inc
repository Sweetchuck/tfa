<?php

class TfaEmail extends TfaBasePlugin implements TfaValidationPluginInterface, TfaSendPluginInterface {

  public function __construct(array $context) {
    parent::__construct($context);
    if (!empty($context['validate_context']) && !empty($context['validate_context']['code'])) {
      $this->code = $context['validate_context']['code'];
    }
  }

  /**
   *
   */
  public function begin() {
    if (!$this->code) {
      $this->code = $this->generate();
      drupal_set_message('gen ' . $this->code);
      $this->sendCode($this->code);
    }
  }

  public function getForm(array $form, array &$form_state) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Verification Code'),
      '#required' => TRUE,
      '#description' => t('Enter @length-character code sent to your device.', array('@length' => $this->codeLength)),
    );
    $form['actions']['login'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );
    $form['actions']['resend'] = array(
      '#type' => 'submit',
      '#value' => t('Resend'),
      '#submit' => array('tfa_form_submit'),
      '#limit_validation_errors' => array(),
    );

    return $form;
  }

  public function validateForm(array $form, array &$form_state) {
    //$this->code = variable_get('tfa_test_email_code', 'code');
    if (!parent::validate($form_state['values']['code'])) {
      $this->errorMessages['code'] = t('Invalid code.');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  public function submitForm(array $form, array &$form_state) {
    // Resend code if pushed.
    if ($form_state['values']['op'] === $form_state['values']['resend']) {
      drupal_set_message('Code resent');
      $this->code = $this->generate();
      drupal_set_message('regen ' . $this->code);
      $this->sendCode($this->code);
      return FALSE;
    }
    else {
      return parent::submitForm($form, $form_state);
    }
  }

  /**
   * Return context for this plugin.
   *
   * @return array
   */
  public function getPluginContext() {
    return array(
      'code' => $this->code,
    );
  }

  protected function generate() {
    $code = parent::generate();
    // store code
    //variable_set('tfa_test', $code);
    $this->code = $code; //variable_set('tfa_test_email_code', $code);
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

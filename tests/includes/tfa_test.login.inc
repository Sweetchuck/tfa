<?php

/**
 * @file
 * Home of TfaTestLogin,
 */

/**
 * Class TfaTestLogin.
 */
class TfaTestLogin extends TfaBasePlugin implements TfaLoginPluginInterface {

  /**
   * @var int
   */
  protected $loginUid;

  /**
   * @param array $context
   *
   * @param int $uid
   */
  public function __construct(array $context = array(), $uid) {
    parent::__construct($context);

    $this->loginUid = $uid;
  }

  /**
   * {@inheritdoc}
   */
  public function loginAllowed() {
    return ($this->context['uid'] == $this->loginUid);
  }
}

# Contributing

This document explains how to contribute to this project using email.

## Sending patches

Please submit patches to `<patches@usebox.net>`. The preferred way to do that is [git send-email](https://git-scm.com/docs/git-send-email) (see [this step-by-step guide](https://git-send-email.io/) for more information).

Alternatively, you can push your changes to a public repository, for example hosted on your own Git server, on Sourcehut, Gitlab or GitHub, and use [git request-pull](https://git-scm.com/docs/git-request-pull) to send a pull request.

If these options don't work for you, just use your mail client to send a mail with a link to your changes and a short description.

### Example: git send-email

This example shows how to prepare a patch. First you have to check out the Git repository and configure the destination email address and the subject prefix:

```
$ git clone https://git.usebox.net/ubox-msx-lib && cd ubox-msx-lib
$ git config sendemail.to "patches@usebox.net"
$ git config format.subjectPrefix "PATCH ubox-msx-lib"
```

Then you can make and commit your changes.

Finally use `git send-email` to generate a patch for the last commit and send it:

```
$ git send-email HEAD^
```

This example assumes that you have already configured [git send-email](https://git-scm.com/docs/git-send-email) for sending mails.

## Contact

Feel free to [contact me](https://www.usebox.net/jjm/about/me/) for questions or to discuss big changes in advance.


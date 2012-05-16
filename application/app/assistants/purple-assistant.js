
var PurpleAssistant = function (params) {
    this.template = params.initialTemplate || params.template;

    this.username = params.username || "";
    this.password = params.password || "";
    this.prpl = this.template.prpl;
    this.prefs = {};
    this.options = {};
    this.optionsModels = {};
}

var _ValidatorAddress = "palm://org.webosinternals.purple.validator/";

var PurpleAssistant.prototype.setup = function() {
    this.controller.get('header').innerHTML = this.template.loc_name;

    // Username
    this.usernameModel = {
        value: this.username,
        disabled: false
    };

    this.controller.setupWidget('username',
        {
            hintText: this.template.loc_usernameExplainLabel || "",
            focus: true
        },
        this.usernameModel
    );

    // Password
    this.passwordModel = {
        value: this.password,
        disabled: false
    };

    this.controller.setupWidget('password',
        {
            textFieldName: "loc_passwordLabel",
            hintText: this.template.loc_passwordExplainLabel || ""
        },
        this.passwordModel
    );

    // Call getOptions
    new Mojo.Service.Request(_ValidatorAddress + "getOptions", {
        parameters: {
            prpl: this.template.prpl
        },
        onSuccess: this.optionsSuccess.bind(this)
        // TODO: onFailure: Return
    });
);

var PurpleAssistant.prototype.optionsSuccess = function(response) {
    this.createOptionsWidget(response.options);

    // Setup CreateAccount widget
    this.controller.setupWidget('CreateAccountButton',
        {
            type: Mojo.Widget.activityButton
        },
        {
            buttonClass: 'primary',
            buttonLabel: 'Sign In',
            disabled: false
        }
    );

    this.controller.listen('CreateAccountButton', Mojo.Event.tap,
            PurpleAssistant.createAccount.bind(this)
        );
);

var PurpleAssistant.prototype.createAccount = function(ev) {
    if (this.usernameModel.value == "")
    {
        this.showError("Validation", "Please enter a valid username");
        return;
    }

    this.disableControls();

    new Mojo.Service.Request(_ValidatorAddress + "validateAccount", {
        parameters: {
            "username": this.usernameModel.value,
            "password": this.passwordModel.value,
            "prpl": this.template.prpl,
            "templateId": this.template.templateId,
            "config": this.prefs
        },
        onFailure: this.loginFailure.bind(this),
        onError: this.loginFailure.bind(this)
    });

    this.getEvent();
};

var PurpleAssistant.prototype.eventSuccess = function(response) {
    if ("credentials" in response)
    {
        var result = response;
        result.username = this.usernameModel.value;
        result.config = this.prefs;

        this.controller.stageController.popScene(result);
    }
    else
    {
        this.showPopup(response);
    }
};

var PurpleAssistant.prototype.eventFail = function(response) {
    if (response.errorCode) {
        // TODO: Show error message
    }
    else
    {
        // TODO: Hide error message
    }

    this.enableControls();
};

var PurpleAssistant.prototype.enableControls = function(val) {
    PurpleAssistant.createButton.mojo.deactivate();
    this.toggleControls(true);
};

var PurpleAssistant.prototype.disableControls = function(val) {
    this.toggleControls(false);
};

var PurpleAssistant.prototype.toggleControls = function(val) {
    val = val && true || false;
    this.usernameModel.disabled = val;
    this.passwordModel.disabled = val;

    this.controller.modelChanged(this.usernameModel);
    this.controller.modelChanged(this.passwordModel);

    for (var name in this.optionsModels)
    {
        this.optionsModels[name].disabled = val;
        this.controller.modelChanged(this.optionsModels[name]);
    }
};

var PurpleAssistant.prototype.showPopup = function(popup) {
    var dialog = {
        message: ""
    };
    if ("title" in popup)
        dialog.title = popup.title;
    if ("primary" in response)
        dialog.message += popup.primary;
    if ("secondary" in response)
    {
        if (dialog.message != "")
            dialog.message += "<br/>";
        dialog.message += popup.secondary;
        dialog.allowHTMLMessage = true;
    }

    if ("actions" in popup)
    {
        var choices = [];
        for (var i in popup.actions)
        {
            choices.push({
                label: popup.actions[i],
                value: {
                    id: popup.actions[i].id,
                    request_id: popup.actions[i].request_id
                }
            });
        }

        dialog.choices = choices;
        dialog.preventCancel = true;

        dialog.handle = function(value) {
            new Mojo.Service.Request(_ValidatorAddress + "answerEvent", {
                parameters: {
                    answer: value.id,
                    id: value.request_id
                },
                onResponse: function() {
                    this.getEvent();
                }
            });
        };

    }
    else if ("type" in popup)
    {
    }

    this.controller.showAlertDialog(dialog);
};

var PurpleAssistant.prototype.getEvent = function() {
    new Mojo.Service.Request(_ValidatorAddress + "getEvent", {
        parameters: {},
        onSuccess: this.eventSuccess.bind(this),
        onFailure: this.eventFailure.bind(this)
    });
};

var _TypeToMojoElement = {
    "string": "TextField",
    "int": "TextField",
    "bool": "CheckBox",
    "list": "ListSelector"
};

var PurpleAssistant.prototype.createOptionsWidget = function(options) {
    var optionsModel = [];

    for (var name in options) {
        var node = options[name];
        if (node.type in _TypeToMojoElement)
        {
            optionsModel.push({
                name: name,
                text: node.text,
                mojo_element: _TypeToMojoElement[node.type]
            });
        }
    }

    this.options = options;

    this.controller.setupWidget("OptionsList", {
            itemTemplate: "templates/options-item",
            swipeToDelete: false,
            autoconfirmDelete: false,
            reorderable: false
        },
        optionsModel);

    for (var name in options) {
        var node = options[name];

        var attributes = {};
        var model = { disabled: false };

        if ("default_value" in node)
            model.value = node.default_value;

        if (node.type == "string")
        {
            attributes.changeOnKeyPress = true;
        }
        else if (node.type == "int")
        {
            attributes.changeOnKeyPress = true;
            attributes.charsAllow = function(c) {
                if (/[0-9]/.test(c))
                    return true;
                else
                    return false;
            }
        }
        else if (node.type == "bool")
        {
        }
        else if (node.type == "list")
        {
            attributes.choices = [];
            for (var name_ in node.choices)
                attributes.choices.push({
                    label: node.choices[name_],
                    value: name_
                });
            if ("default_value" in node)
                model.value = node.default_value
        }

        this.optionsModels[name] = model;

        this.controller.setupWidget(name, attributes, this.optionsModels[name]);
        Mojo.Event.listen(
                this.controller.get(name),
                Mojo.Event.propertyChange,
                function (ev) {
                    this.prefs[name] = ev.value;
                }
        );
    }
};


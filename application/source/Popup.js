enyo.kind({
    name: "Purple.PopupGenerator",
    kind: "Control",

    events: {
        onAction: ""
    },

    published: {
        request_id: 0
    },

    showPopup: function(response) {
        if (this.$._popup)
            this.$._popup.destroy();

        var innerComponents = new Array();

        if ("title" in response)
        {
            innerComponents.push({
                content: response.title
            })
        }

        if ("primary" in response)
        {
            // TODO: Smaller font
            innerComponents.push({
                content: response.primary
            });
        }

        if ("secondary" in response)
        {
            // TODO: Even smaller font
            innerComponents.push({
                content: response.secondary
            })
        }

        if ("actions" in response)
        {
            var actions = new Array();
            for (var i in response.actions)
            {
                actions.push({
                    kind: "Button",
                    caption: response.actions[i],
                    onclick: "handleClick",
                    index: Number(i),
                    request_id: Number(response.id)
                });
            }

            innerComponents.push({
                layoutKind: "HFlexLayout",
                pack: "center",
                components: actions
            });
        }
        else if ("type" in response)
        {
            innerComponents.push({
                kind: "Button",
                caption: "OK"
            });
        }
        else
            return;

        this.createComponent({
            name: "_popup",
            kind: "ModalDialog",
            onAction: "doAction",
            components: innerComponents
        });

        this.$._popup.openAtCenter();
    },

    handleClick: function(inSender) {
        this.$._popup.close();
        if ("request_id" in inSender)
            this.doAction({
                id: inSender.request_id,
                answer: inSender.index
            });
    }

})

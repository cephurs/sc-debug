//
//  LiveCodingView.h
//  isclang
//
//  Created by Axel Balley on 30/10/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface LiveCodingView : UIView <UITextViewDelegate>
{
	IBOutlet UITextField *textField;
	IBOutlet UITextView *textView;
	IBOutlet UIButton *doneButton;
	IBOutlet UIButton *lineButton;
	IBOutlet UIButton *blockButton;

	id target;
	SEL selector;
}
- (void) setTarget:(id)t withSelector:(SEL)s;
- (void) loadFile:(NSString *)file;
- (void) showButtons: (BOOL)state;
- (IBAction) triggerDone: (id)sender;
- (IBAction) triggerLine: (id)sender;
- (IBAction) triggerBlock: (id)sender;
- (IBAction) triggerExecute: (id)sender;

@end
